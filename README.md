# C++ L530 API

This C++ implementation of the TP Link L530 Light bulb API was desgined for an ESP32. This is an example class which I later use on a ESP32 CYD with a UI to control the smart bulbs within my house. This was my first project which has been written in C++ so there is likely to be bad practices and alot of code improvements that can be made.

## Credentials

To use this program you will need to encode your TP LINK username and password with SHA1 Hashing. this can be done with a simple python script like 

```python 
import hashlib, os

def sha1(value):
    return hashlib.sha1(value).digest()

username = os.getenv("username")
password = os.getenv("password")

encoded = sha1(username.encode()) + sha1(password.encode())

with open('creds.bin', 'wb') as f:
    f.write(encoded)

```

With this code an .env file is needed with the fields layed out like 

```
username = "Email@Email.com"
password = "PASSWORD"

```

Or the Lighting Class can take creds as an data field in its constructor it will need to be std::vector<uint8_t> type.

The Output file can then be uploaded onto the ESP32 using the Build FileSystem Image option on PlatformIO the file needs to be stored in a dir named /data

## Run Locally

Clone the project

```bash
  git clone https://github.com/Infinity585/CPP-L530-API.git
```

Go to the project directory

```bash
  cd CPP-L530-API
```

Install PlatformIO
- [PlatformIO Install](https://platformio.org/install/ide?install=vscode)

Complete the setup by building the project though PlatformIO and then finally upload the code onto the board.



## Acknowledgements

 - [My Python Implementation](https://github.com/Infinity585/Python-L530-API)

This code was based on my python implemenation which was developed based on the following projects.
 - [Original TP Link P100](https://github.com/K4CZP3R/tapo-p100-python)
 - [Maintained Python P100 Fork ](https://github.com/petretiandrea/plugp100)
 - [Rust and Python TAPO API](https://github.com/mihai-dinculescu/tapo)


## Contributing

Contributions are always welcome!
 as well as feed back.
