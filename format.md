# Big file format

#### Header beginning
* First 4 bytes are _BIGF_ indicating that it's a big file
* The other 4 bytes indicate the _EOF_ location
* The other 4 bytes indicate how many files there are, but the number is written in reversed
* The last 4 bytes indicate end of header, but the number is written in reversed

#### Header content
* 4 bytes indicating start of data stream, but the number is written in reversed
* 4 bytes indicating stream length, but the number is written in reversed
* Null-terminated string indicating file name

#### Heander end
* 8 bytes containing _L255\0\0\0\0_, unknown purpuse
