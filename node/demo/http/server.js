const express = require('express');
const ws = require('ws');

const o = require('../../build/Release/vcam.node')
const option = require(`${__dirname}/../../../bin/Vcam.json`);


var d;
console.log('init');
do {
    d = o.init(option);
} while(!d.success || d.width === 0)
console.log(d);

const app = express();

app.use('/', express.static(`${__dirname}/public`));

const wsServer = new ws.Server({ noServer: true });
wsServer.on('connection', socket => {
  socket.on('message', message => {
      console.log(message.length);
      //var d2 = o.update(Buffer.from(message, 'base64'));
      var d2 = o.update(message);
      if (   d2.width !== d.width 
        || d2.height !== d.height 
        || d2.bitcount !== d.bitcount
        || d2.success === false)  {
            d = d2;
            socket.send(JSON.stringify(d));
      }
  });
  socket.send(JSON.stringify(d));
});


const server = app.listen(3000);
server.on('upgrade', (request, socket, head) => {
  wsServer.handleUpgrade(request, socket, head, socket => {
    wsServer.emit('connection', socket, request);
  });
});