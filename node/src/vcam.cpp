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
    bool r = _camsource.init(&bi);
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    Nan::Set(obj, Nan::New("success").ToLocalChecked(), Nan::New<v8::Boolean>(r));
    Nan::Set(obj, Nan::New("width").ToLocalChecked(), Nan::New<v8::Number>(bi.biWidth));
    Nan::Set(obj, Nan::New("height").ToLocalChecked(), Nan::New<v8::Number>(bi.biHeight));
    Nan::Set(obj, Nan::New("bitcount").ToLocalChecked(), Nan::New<v8::Number>(bi.biBitCount));

    
    info.GetReturnValue().Set(obj);
}

NAN_METHOD(info) {
}

NAN_METHOD(update) {
    BYTE* buf = (BYTE *)_camsource.get(&bi);
    unsigned size = bi.biWidth * bi.biHeight * bi.biBitCount / 8;
    BYTE *data = ((BYTE *)node::Buffer::Data(Nan::To<v8::Object>(info[0]).ToLocalChecked()));

    memcpy(buf,data,size);
    //for (unsigned i = 0; i < size; ++i) buf[i] = rand();

    _camsource.release();
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    Nan::Set(obj, Nan::New("width").ToLocalChecked(), Nan::New<v8::Number>(bi.biWidth));
    Nan::Set(obj, Nan::New("height").ToLocalChecked(), Nan::New<v8::Number>(bi.biHeight));
    Nan::Set(obj, Nan::New("bitcount").ToLocalChecked(), Nan::New<v8::Number>(bi.biBitCount));

    info.GetReturnValue().Set(obj);
}

NAN_METHOD(term) {}

NAN_MODULE_INIT(InitAll) {
    Nan::Set(target, Nan::New("init").ToLocalChecked(), Nan::GetFunction(Nan::New<v8::FunctionTemplate>(init)).ToLocalChecked());
    Nan::Set(target, Nan::New("update").ToLocalChecked(), Nan::GetFunction(Nan::New<v8::FunctionTemplate>(update)).ToLocalChecked());
    Nan::Set(target, Nan::New("term").ToLocalChecked(), Nan::GetFunction(Nan::New<v8::FunctionTemplate>(term)).ToLocalChecked());
}

NODE_MODULE(vcam, InitAll)