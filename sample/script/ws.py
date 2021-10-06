import os
import sys
import time
import cv2
import numpy as np
from tornado.web import RequestHandler, Application
from tornado.websocket import WebSocketHandler, websocket_connect
from tornado.httpserver import HTTPServer
from tornado.ioloop import IOLoop, PeriodicCallback
from tornado import gen

LISTEN_PORT = 8000
WSOCKET_PORT = 3000
WSOCKET_URL = 'localhost'

WCMD_OPEN_R = 1
WCMD_OPEN_W = 2
WCMD_CLOSE = 3
WCMD_READ = 4
WCMD_WRITE = 5
WCMD_SEEK = 6
WCMD_SIZE = 7

FILE = 0
PATH = 1


class SocketHandler(object):
    def __init__(self, url, client):
        self.url = url
        self.ws = None
        self.client = client
        self.files = []
        self.fd = None
        self.image = None
        self.connect()
        self.pcb = PeriodicCallback(self.keep_alive, 5000)
        self.pcb.start()

    @gen.coroutine
    def connect(self):
        try:
            print('trying to connect')
            self.ws = yield websocket_connect(self.url)
            print('connected')
            self.run()
        except Exception as e:
            print('connection error')

    def keep_alive(self):
        if self.ws is None:
            self.connect()

    def isconnected(self):
        return self.ws != None

    def close(self):
        self.pcb.stop()
        self.ws.write_message('')  # send close message
        self.ws = None

    def send_image(self, name, path, image):
        image = np.fromstring(image, dtype='uint8')
        image = cv2.imdecode(image, cv2.IMREAD_COLOR)
        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGBA)
        self.client.send(name + ';' + str(image.shape[0]) + ';' +
                         str(image.shape[1]) + ';' + path)
        self.client.send(image.tobytes(), binary=True)

    def get_fd(self):
        if None in self.files:
            return self.files.index(None)
        return len(self.files)

    def set_fd(self, fd, item):
        if fd >= len(self.files):
            self.files.append(item)
        else:
            self.files[fd] = item

    @gen.coroutine
    def handle(self):
        try:
            msg = yield self.ws.read_message()
            if msg is None:
                print('connection closed')
                self.ws = None

            elif isinstance(msg, bytes):  # file operation
                msg = int.from_bytes(msg, 'little')
                cmd = msg >> 16
                fd = msg & 0x0ffff

                if cmd == WCMD_OPEN_R:
                    fd = self.get_fd()
                    path = yield self.ws.read_message()

                    if os.path.exists(path):
                        file = open(path, 'rb')
                        self.set_fd(fd, [file, path])
                    else:
                        print('file not found:', path)

                    self.ws.write_message(fd.to_bytes(4, 'little'),
                                          binary=True)

                    print('open_r', fd)

                elif cmd == WCMD_OPEN_W:
                    fd = self.get_fd()
                    path = yield self.ws.read_message()
                    file = None
                    #file = open(path,'wb') # Uncomment if really save file
                    self.set_fd(fd, [file, path])

                    self.ws.write_message(fd.to_bytes(4, 'little'),
                                          binary=True)

                elif cmd == WCMD_CLOSE:
                    self.files[fd] = None

                elif cmd == WCMD_READ:
                    msg = yield self.ws.read_message()
                    size = int.from_bytes(msg, 'little')
                    msg = self.files[fd][FILE].read(size)

                    # wait next
                    while self.isconnected() and not self.client.next:
                        yield time.sleep(0.03)

                    self.ws.write_message(msg, binary=True)
                    self.image = msg
                    self.fd = fd

                elif cmd == WCMD_WRITE:
                    msg = yield self.ws.read_message()
                    if self.files[fd][FILE]: self.files[fd][FILE].write(msg)

                    self.send_image('c1', self.files[fd][PATH], msg)
                    if self.fd:
                        self.send_image('c0', self.files[self.fd][PATH],
                                        self.image)
                        self.fd = None

                elif cmd == WCMD_SEEK:
                    msg = yield self.ws.read_message()
                    pos = int.from_bytes(msg, 'little')
                    if self.files[fd][FILE]: self.files[fd][FILE].seek(pos)

                elif cmd == WCMD_SIZE:
                    size = os.path.getsize(self.files[fd][PATH])
                    self.ws.write_message(size.to_bytes(4, 'little'),
                                          binary=True)

            elif isinstance(msg, str):  # other operation

                if msg == 'INIT':
                    msg = yield self.ws.read_message()
                    self.client.send('title;' + msg)
                    print('title', msg)

                elif msg == 'PRINT':
                    msg = yield self.ws.read_message()
                    self.client.send('result;' + msg)
                    print('result', msg, end='', flush=True)
                    if self.fd:
                        self.send_image('c0', self.files[self.fd][PATH],
                                        self.image)
                        self.fd = None

                else:
                    print('info', msg)
            else:
                print('Invalid datadata received', type(msg))
        except Exception as e:
            print(e)

    @gen.coroutine
    def run(self):
        while self.isconnected():
            yield self.handle()


class ClientHandler(WebSocketHandler):
    def __init__(self, *args, **kwargs):
        super(ClientHandler, self).__init__(*args, **kwargs)

    def check_origin(self, origin):
        return True

    def open(self):
        self.next = False
        self.socket = SocketHandler(
            'ws://' + WSOCKET_URL + ':' + str(WSOCKET_PORT), self)

    def on_close(self):
        print('on_close')
        self.socket.close()

    def on_message(self, message):
        self.next = True

    def send(self, message, binary=False):
        self.write_message(message, binary=binary)
        if binary: self.next = False


class WebHandler(RequestHandler):
    def get(self):
        if self.request.uri == '/':
            self.render('ws.html')


# Create tornado application and supply URL routes
application = Application(handlers=[(r'/', WebHandler),
                                    (r'/ws', ClientHandler)],
                          static_path=os.path.join(os.path.dirname(__file__),
                                                   '.'),
                          template_path=os.path.join(os.path.dirname(__file__),
                                                     '.'))

if __name__ == '__main__':

    WSOCKET_URL = sys.argv[1] if len(sys.argv) > 1 else WSOCKET_URL

    http_server = HTTPServer(application)
    http_server.listen(LISTEN_PORT)

    print('Server is listening on port %d' % LISTEN_PORT)

    IOLoop.instance().start()
