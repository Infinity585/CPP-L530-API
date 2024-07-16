#include "lighting.h"

/**
 * @brief Constructor for the lighting class
 * @param url The URL of light ending with /app (eg. http://192.168.1.10/app)
 * @return lighting The lighting object
*/
lighting::lighting(String url) {
    this->url = url;
    this->creds = SHA256Hash(loadFile("/creds.bin"));
    Serial.println("Begining Handshake");
    boolean success = handShake();
    while (!success) {
        Serial.println("Handshake failed trying again");
        success = handShake();
    }
    Serial.println("Handshake successful");
    encryptionSetup();
    Serial.println("Encryption setup successful");
}

/**
 * @brief Constructor for the lighting class
 * @param url The URL of light ending with /app (eg. http://192.168.1.10/app)
 * @param creds Your tp-link credentials as a vector of bytes username + password both encoded individually in SHA1
 * @return lighting The lighting object
*/
lighting::lighting(String url , std::vector<uint8_t> creds) {
    this->url = url;
    this->creds = creds;
    boolean success = handShake();
    while (!success) {
        Serial.println("Handshake unsuccessful trying again");
        success = handShake();
        sleep(15); // Waiting 15 seconds before trying again
    }
    
    encryptionSetup();
    Serial.println("Lightbulb setup complete");
}

/**
* @brief Loads bytes from a file
* @param filePath the path to the file as a string
* @return data the bytes stored on the set file
*/

std::vector<uint8_t> lighting::loadFile(String filePath) {
    if (!SPIFFS.begin()) {
        Serial.println("An error has occurred while mounting SPIFFS");
        return std::vector<uint8_t>();
    }
    File file = SPIFFS.open(filePath, "r");
    if (!file) {
        return std::vector<uint8_t>();
    }
    std::vector<uint8_t> data(file.size());
    if (file.available()) {
        file.read(reinterpret_cast<uint8_t*>(&data[0]), file.size());
    }
    file.close();
    return data;
}

/**
* @brief Hashes the given data using SHA256
* @param data The data to hash
* @return hash The SHA256-bit hash of the data
*/
std::vector<uint8_t> lighting::SHA256Hash(const std::vector<uint8_t>& data) {
    SHA256 sha256;
    std::vector<uint8_t> hash(SHA256::HASH_SIZE);
    sha256.reset();
    sha256.update(data.data(), data.size());
    sha256.finalize(hash.data(), SHA256::HASH_SIZE);
    return hash;
}

/**
* @brief Checks the response from the server is 200 OK (this was mostly used for debugging but its easier then repeating)
* @param responsecode The response code from the server as int
* @return boolean True if the response code is 200 OK
*/
boolean lighting::checkResponse(int responseCode) {
    return responseCode == 200;
}

/**
* @brief Generates a random 16 byte seed for the local device
* @return randomBytes A vector of 16 random bytes
*/
std::vector<uint8_t> lighting::genLocalSeed() {
    std::vector<uint8_t> randomBytes(16);
    for (int i = 0; i < 16; i++) {
        randomBytes[i] = random(0, 256); // Generating a long between 0 and 256
    }
    return randomBytes;
}

/**
* @brief Sends the handshake request to the server gathering local hash used for key generation.
* @return boolean True if the handshake was successful
*/
boolean lighting::handShake() {
    HTTPClient http; // setting up http client for post request
    String fullurl = this->url + "/handshake1"; // setting url from the constructor + the endpoint
    http.begin(fullurl); // begin the request

    //The remote sends back a cookie needed for all requests so saving the "set-cookie" header
    const char* headerKeys[] = {"Set-Cookie"};
    size_t headerKeysSize = sizeof(headerKeys) / sizeof(char*);
    http.collectHeaders(headerKeys, headerKeysSize);

    // Sending the request with a random 16 byte local seed.
    std::vector<uint8_t> localSeed = genLocalSeed();
    if(!checkResponse(http.POST(localSeed.data(), localSeed.size()))){
        return false; // Response failed so return false indicating the handshake failed
    }

    // Saving the SessionID from the remote response headers
    std::string cookieStd = (http.header("Set-Cookie")).c_str();
    std::string sessionId = cookieStd.substr(0, cookieStd.find(';'));
    this->cookie = sessionId.c_str();

    //Getting the remote seed from the body of the remote response.
    std::string responseStd = (http.getString()).c_str();
    std::vector<uint8_t> remote_seed(responseStd.begin(), responseStd.begin()+ 16);

    //Preparing the payload for handshake 2
    std::vector<uint8_t> combined_seed(remote_seed);
    combined_seed.insert(combined_seed.end(), localSeed.begin(), localSeed.end());
    combined_seed.insert(combined_seed.end(), this->creds.begin(), this->creds.end());

    //Hashing the combined seed and saving it as the payload for handshake 2
    std::vector<uint8_t> payload = SHA256Hash(combined_seed);
    fullurl = this->url + "/handshake2"; // setting url from the constructor + the endpoint
    http.begin(fullurl); // begin the request for handshake 2
    http.addHeader("Cookie", this->cookie); // adding the SessionID cookie to the headers
    if(!checkResponse(http.POST(payload.data(), payload.size()))){
        return false; // Response failed so return false indicating the handshake failed
    }
    http.end();
    //if the request was sucessful now save the localhash worked out from localseed + remoteseed + creds
    std::vector<uint8_t> localhash(localSeed);
    localhash.insert(localhash.end(), remote_seed.begin(), remote_seed.end());
    localhash.insert(localhash.end(), this->creds.begin(), this->creds.end());
    this->localHash = localhash;

    return true; // Handshake was successful returning.
}

/**
 * @brief Sets up the encryption key, signature, IV and sequence number
*/
void lighting::encryptionSetup() {
    std::vector<uint8_t> local_hash = this->localHash;
    // Key generation
    std::vector<uint8_t> temp({'l', 's', 'k'});
    temp.insert(temp.end(), local_hash.begin(), local_hash.end());
    temp = SHA256Hash(temp);
    std::vector<uint8_t> key(temp.begin(), temp.begin() + 16);
    this->key = key;
    key.clear();
    temp.clear();

    // Encryption signature
    temp.assign({'l', 'd', 'k'});
    temp.insert(temp.end(), local_hash.begin(), local_hash.end());
    temp = SHA256Hash(temp);
    std::vector<uint8_t> sig(temp.begin(), temp.begin() + 28);
    this->sig = sig;
    sig.clear();
    temp.clear();

    // IV and sequence
    temp.assign({'i', 'v'});
    temp.insert(temp.end(), local_hash.begin(), local_hash.end());
    temp = SHA256Hash(temp);
    std::vector<uint8_t> iv(temp.begin(), temp.begin() + 12);
    this -> iv = iv;
    iv.clear();

    uint32_t seq = (temp[temp.size() - 4] << 24) | (temp[temp.size() - 3] << 16) | (temp[temp.size() - 2] << 8) | temp[temp.size() - 1];
    this->seq = seq;
}

/**
 * @brief Converts the sequence number to bytes
 * @return result The sequence number as bytes
*/
std::vector<uint8_t> lighting::seq_to_bytes() {
    uint32_t seq = this->seq;
    std::vector<uint8_t> result;
    result.push_back((seq >> 24) & 0xFF);
    result.push_back((seq >> 16) & 0xFF);
    result.push_back((seq >> 8) & 0xFF);
    result.push_back(seq & 0xFF);
    return result;
}

/**
 * @brief Combines the IV and sequence number into a single vector
 * @return result The IV and sequence number combined
*/
std::vector<uint8_t> lighting::iv_seq() {
    std::vector<uint8_t> result(this->iv); // Start with the initial IV
    std::vector<uint8_t> seqBytes = seq_to_bytes();
    result.insert(result.end(), seqBytes.begin(), seqBytes.end());
    return result; // Return the resulting iv_seq
}

/**
 * @brief Pads the data to the block size using PKCS7 padding
 * @param data The data to pad
 * @param block_size The block size to pad to (16 for AES)
 * @return padded_data The padded data
*/
std::vector<uint8_t> lighting::pkcs7_pad(const std::vector<uint8_t>& data, size_t block_size) {
    size_t pad_len = block_size - (data.size() % block_size);
    std::vector<uint8_t> padded_data = data;
    padded_data.insert(padded_data.end(), pad_len, pad_len);
    return padded_data;
}

// XOR function for CBC mode encryption (CODE GENERTAED WITH CHATGPT)
void lighting::xorBlock(uint8_t* dst, const uint8_t* src, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        dst[i] ^= src[i];
    }
}

/**
 * @brief Encrypts the given data using AES128 in CBC mode
 * @param data The data to encrypt
 * @return result The encrypted data as bytes
*/
std::vector<uint8_t> lighting::encrypt(const String& data) {
    this->seq++;
    std::vector<uint8_t> iv_seq_data = iv_seq();

    AES128 aesEncryptor;
    aesEncryptor.setKey(this->key.data(), this->key.size());

    std::vector<uint8_t> padded_data = pkcs7_pad(std::vector<uint8_t>(data.begin(), data.end()), 16);

    // This Code was generated using CHATGPT I was stuck on the AES trying to get it to use CBC mode.
    std::vector<uint8_t> cipher_bytes(padded_data.size());
    uint8_t current_iv[16];
    memcpy(current_iv, iv_seq_data.data(), 16);

    for (size_t i = 0; i < padded_data.size(); i += 16) {
        xorBlock(padded_data.data() + i, current_iv, 16);
        aesEncryptor.encryptBlock(cipher_bytes.data() + i, padded_data.data() + i);
        memcpy(current_iv, cipher_bytes.data() + i, 16);
    }
    // End of CHATGPT generated code

    // Preparing the signature by adding the sig and the sequency as bytes together then finally adding the cipher bytes
    std::vector<uint8_t> signature_input = sig;
    std::vector<uint8_t> seq_bytes = seq_to_bytes();
    signature_input.insert(signature_input.end(), seq_bytes.begin(), seq_bytes.end());
    signature_input.insert(signature_input.end(), cipher_bytes.begin(), cipher_bytes.end());

    // Hashing the signature input
    std::vector<uint8_t> signature = SHA256Hash(signature_input);
    // Adding the cipher bytes to the end of the SHA256 hashed signature.
    std::vector<uint8_t> result = signature;
    result.insert(result.end(), cipher_bytes.begin(), cipher_bytes.end());


    return result; // returning the encrypted data as bytes
}

/**
 * @brief Decrypts the given encrypted data using AES128 in CBC mode and returns the decrypted data as a String
 * @param encryptedData The encrypted data as bytes
 * @return result The decrypted data as a String
 */
String lighting::decrypt(const std::vector<uint8_t>& encryptedData) {
    // Extract the signature and the cipher bytes
    std::vector<uint8_t> signature(encryptedData.begin(), encryptedData.begin() + SHA256::HASH_SIZE);
    std::vector<uint8_t> cipher_bytes(encryptedData.begin() + SHA256::HASH_SIZE, encryptedData.end());

    // Generate the IV sequence
    std::vector<uint8_t> iv_seq_data = iv_seq();

    // Set up the AES decryption
    AES128 aesDecryptor;
    aesDecryptor.setKey(this->key.data(), this->key.size());

    std::vector<uint8_t> decrypted_bytes(cipher_bytes.size());
    uint8_t current_iv[16];
    memcpy(current_iv, iv_seq_data.data(), 16);

    // Decrypt the cipher bytes
    for (size_t i = 0; i < cipher_bytes.size(); i += 16) {
        aesDecryptor.decryptBlock(decrypted_bytes.data() + i, cipher_bytes.data() + i);
        xorBlock(decrypted_bytes.data() + i, current_iv, 16);
        memcpy(current_iv, cipher_bytes.data() + i, 16);
    }

    // Remove PKCS7 padding
    size_t pad_len = decrypted_bytes.back();
    decrypted_bytes.resize(decrypted_bytes.size() - pad_len);

    // Convert decrypted bytes to String
    String decryptedString(reinterpret_cast<const char*>(decrypted_bytes.data()), decrypted_bytes.size());

    return decryptedString;
}

LightResult lighting::executeRequest(String data){
    LightResult result;

    HTTPClient http; // setting up the http client for the post request
    std::vector<uint8_t> payload = encrypt(data); // Encrypting the payload (must be done before the endpoint is set since seq++.)


    String fullurl = this->url + "/request?seq=" + String(this->seq); //setting url + endpoint and adding the sequence number
    http.begin(fullurl); // begining the http request at the endpoint
    http.addHeader("Cookie", this->cookie); // adding cookies to the headers
    

    if (!checkResponse(http.POST(payload.data(), payload.size()))) {
        result.success = false; 
        return result; // Request failed returning with success = false
    }


    // Reading the response from the server I orginally tried to use http.getString()
    // The issue was for the getinfo returns too much infomation so a stream was required.
    WiFiClient* stream = http.getStreamPtr();
    std::vector<uint8_t> response;
    size_t size = http.getSize();
    
    response.reserve(size);
    uint8_t buffer[128];
    
    while (http.connected() && size > 0) {
        size_t bytesRead = stream->readBytes(buffer, sizeof(buffer));
        response.insert(response.end(), buffer, buffer + bytesRead);
        size -= bytesRead;
    }

    String decryptStr = decrypt(response);


    result.success = true; // reporting that everything was sucessful 
    result.response = decryptStr; // setting the response as the string decrypted
    
    return result; // returning the results.

}

LightResult lighting::toggleLight(boolean lightState) {
    
    //Creating the JSON payload
    JsonDocument doc;

    doc["method"] = "set_device_info";
    doc["params"]["device_on"] = true;
    
    String data;
    doc.shrinkToFit(); 
    serializeJson(doc, data);
    
    //Executing the request and storing the result
    LightResult result = executeRequest(data);
    return result;
}

LightInfo lighting::getLightInfo(){
    //Creating the JSON payload
    JsonDocument doc;
    doc["method"] = "get_device_info";
    String data;
    doc.shrinkToFit(); 
    serializeJson(doc, data);

    //Executing the request and storing the result
    LightResult decryptedData = executeRequest(data);

    LightInfo lightInfo;
    StaticJsonDocument<1024> doc;
     
    DeserializationError error = deserializeJson(doc, decryptedData.response);
    if(error){
        Serial.println("Error parsing JSON");
        return lightInfo;
    }

    JsonObject result = doc["result"];
    lightInfo.device_on = result["device_on"];
    lightInfo.brightness = result["brightness"];
    lightInfo.hue = result["hue"];
    lightInfo.saturation = result["saturation"];
    lightInfo.color_temp = result["color_temp"];

    return lightInfo;


}