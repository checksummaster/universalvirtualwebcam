#include <node.h>
#include <node_buffer.h>
#include <nan.h>

#include <windows.h>
#include "camsource.h"
camsource _camsource;
BITMAPINFOHEADER bi;

int anim = 1;
int line = 0;

NAN_METHOD(init) {

    v8::Local<v8::Object> input = v8::Local<v8::Object>::Cast(info[0]);
    Nan::Utf8String memnamedata(Nan::Get(input, Nan::New("data").ToLocalChecked()).ToLocalChecked());
    Nan::Utf8String memnameconfig(Nan::Get(input, Nan::New("config").ToLocalChecked()).ToLocalChecked());
    Nan::Utf8String memnamelock(Nan::Get(input, Nan::New("lock").ToLocalChecked()).ToLocalChecked());


    bool r = _camsource.init(&bi,*memnamedata,*memnameconfig,*memnamelock);
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    Nan::Set(obj, Nan::New("success").ToLocalChecked(), Nan::New<v8::Boolean>(r));
    Nan::Set(obj, Nan::New("width").ToLocalChecked(), Nan::New<v8::Number>(bi.biWidth));
    Nan::Set(obj, Nan::New("height").ToLocalChecked(), Nan::New<v8::Number>(bi.biHeight));
    Nan::Set(obj, Nan::New("bitcount").ToLocalChecked(), Nan::New<v8::Number>(bi.biBitCount));
    info.GetReturnValue().Set(obj);
}

NAN_METHOD(update) {
    unsigned expectedsize = bi.biWidth * bi.biHeight * bi.biBitCount / 8;
    BYTE* buf = (BYTE *)_camsource.get(&bi);
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
 
    if (buf) {
        unsigned size = bi.biWidth * bi.biHeight * bi.biBitCount / 8;        
        if (expectedsize == size) {            
            BYTE *data = ((BYTE *)node::Buffer::Data(Nan::To<v8::Object>(info[0]).ToLocalChecked()));
            memcpy(buf,data,size);
        }
        _camsource.release();        
        Nan::Set(obj, Nan::New("success").ToLocalChecked(), Nan::New<v8::Boolean>(true));
        Nan::Set(obj, Nan::New("width").ToLocalChecked(), Nan::New<v8::Number>(bi.biWidth));
        Nan::Set(obj, Nan::New("height").ToLocalChecked(), Nan::New<v8::Number>(bi.biHeight));
        Nan::Set(obj, Nan::New("bitcount").ToLocalChecked(), Nan::New<v8::Number>(bi.biBitCount));        

    } else {
        Nan::Set(obj, Nan::New("success").ToLocalChecked(), Nan::New<v8::Boolean>(false));
        Nan::Set(obj, Nan::New("width").ToLocalChecked(), Nan::New<v8::Number>(0));
        Nan::Set(obj, Nan::New("height").ToLocalChecked(), Nan::New<v8::Number>(0));
        Nan::Set(obj, Nan::New("bitcount").ToLocalChecked(), Nan::New<v8::Number>(0));
    }
    info.GetReturnValue().Set(obj);
}

NAN_METHOD(term) {
    _camsource.close();
}



NAN_MODULE_INIT(InitAll) {
    Nan::Set(target, Nan::New("init").ToLocalChecked(), Nan::GetFunction(Nan::New<v8::FunctionTemplate>(init)).ToLocalChecked());
    Nan::Set(target, Nan::New("update").ToLocalChecked(), Nan::GetFunction(Nan::New<v8::FunctionTemplate>(update)).ToLocalChecked());
    Nan::Set(target, Nan::New("term").ToLocalChecked(), Nan::GetFunction(Nan::New<v8::FunctionTemplate>(term)).ToLocalChecked());
}

NODE_MODULE(vcam, InitAll)