## Simple WebSocket File System

__setup for web server__
```
$ pip install tornado
$ pip install opencv-python
```

__sample build__
```
$ cd sample
$ mkdir build
$ cd build
$ cmake ..
$ make
```

__websocket server__
```
$ cd sample
$ ./build/wsocket
```

__web server__
```
$ cd sample
$ python ./script/ws.py
```

__access to http://(hostURL):8000__
