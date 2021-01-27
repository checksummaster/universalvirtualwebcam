var o = require('./build/Release/vcam.node')
var option = require(`${__dirname}/../bin/Vcam.json`);

var prec = 2;
const height=80*prec;    
const width=100*prec;   
const die = 0.000;//0.04/prec;
const skip = 10;
let fire=new Array(width * height).fill(0); 

function burn(twidth,theight,buffer) {
    for(var i=0; i<width; i++)
            fire[i+width]=Math.random()*255; 

    for(var y=height; y>1; y--)           
            for(var x=0; x<width; x++) {  
                    var i=y*width+x;   
                    fire[i]=Math.floor((                
                    fire[(y-1)*width+        (x-1+width)%width]+
                    fire[(y-1)*width+        (x  +width)%width]+
                    fire[(y-1)*width+        (x+1+width)%width]+
                    fire[(y-2)*width+        (x  +width)%width]
                    )/(4 + die));}
    for (var y = 0; y < theight ; y++) {
        for (var x = 0; x < twidth ; x++) {
            var tx = Math.floor(x*width/twidth);
            var ty = Math.floor(y*(height-skip)/theight)+skip;
            var v = fire[ty * width + tx];
     
            buffer[(y*twidth+x)*3+2] = v ;
            buffer[(y*twidth+x)*3+1] = 0;
            buffer[(y*twidth+x)*3+0] = 0;
        }
    }
}

var d;

console.log('init');
do {
    d = o.init(option);
} while(!d.success || d.width === 0)

console.log(d);
var buffer = Buffer.alloc(d.width * d.height * d.bitcount/8); 
while(d.success) {
    burn(d.width , d.height, buffer);
    var d2 = o.update(buffer);
    if (   d2.width !== d.width 
        || d2.height !== d.height 
        || d2.bitcount !== d.bitcount
        || d2.success === false) {
            console.log('break ',d2);
            d = d2;
            buffer = Buffer.alloc(d.width * d.height * d.bitcount/8); 
        }
    console.log(d);
}
o.term();
