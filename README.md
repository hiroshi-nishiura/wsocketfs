## Simple WebSocket File System

組み込みデバイスでイメージ処理系のプログラムを開発したいが、ファイルシステムがなかったり、ディスプレイがなかったりすることは往々にしてある。
それでもネットワークは使えるというときのデバッグ開発環境として、WebSocketを通して外部ファイルにアクセスして、ブラウザをディスプレイ代わりにできれば便利だと思って作ったもの。
デバイス側のネットワークはSocketさえ使えればOK。ESP32でも動く。

__仕組み__
[組み込みデバイス]
--wsocketfs--
[WebSocketServer][WebServer]
--WebSocket--
[Browser]

#### Build

sample/sample.c：組み込み向けターゲットプログラムサンプル
```
$ cd sample
$ mkdir build
$ cd build
$ cmake ..
$ make
```
__run__
```
$ cd sample
$ ./build/sample
```

#### WebSocket and WebServer

ターゲットプログラムとブラウザ向けのサーバープログラム
ターゲットからのリクエストを受けて実際のファイルアクセスするハンドラー
ターゲットで処理したイメージデータを受信してブラウザに送るハンドラー

__requirements__
```
$ pip install tornado
$ pip install opencv-python
```
__run__
```
$ cd sample
$ python ./script/server.py
```

__access to http://(hostURL):8000__
