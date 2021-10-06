var socket = null
var rect = {}
var name = 'c0'

function onload() {
  pause = false

  socket = new WebSocket('ws://' + document.location.host + '/ws')
  socket.binaryType = 'arraybuffer'

  socket.onopen = function () {
    // for(c in ['c0','c1']) {
    //     var canvas = document.getElementById(c);
    //     canvas.width = 1;
    //     canvas.height = 1;
    // }
    document.onkeydown = keydown
    next()
  }

  socket.onmessage = function (message) {
    if (typeof message.data == 'string') {
      args = message.data.split(';')
      name = args[0]
      if (name == 'c0') {
        setText('src', args[3])
        rect = [Number(args[1]), Number(args[2])]
      }
      else if (name == 'c1') {
        setText('dst', args[3])
        rect = [Number(args[1]), Number(args[2])]
      }
      else {
        setText(name, args[1])
      }
    }
    else {
      var canvas = document.getElementById(name)
      canvas.width = rect[0]
      canvas.height = rect[1]
      var ctx = canvas.getContext('2d')
      var imgData = ctx.createImageData(canvas.width, canvas.height)
      imgData.data.set(new Uint8ClampedArray(message.data))
      ctx.putImageData(imgData, 0, 0)
      next()
    }
  }

  function next() {
    if (!pause) {
      setText('bottom', "running")
      socket.send('next')
    }
    else {
      setText('bottom', 'paused')
    }
  }

  function setText(elem, text) {
    var e = document.getElementById(elem)
    if (e) e.textContent = text
  }

  function keydown() {
    switch (event.keyCode) {
      case 32:
        pause = !pause
        next()
        break
    }
  }
}
