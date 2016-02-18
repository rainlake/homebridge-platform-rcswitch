#include <node.h>
#include <v8.h>
#include <uv.h>
#include <unistd.h>
#include "rcswitch.h"

using namespace v8;

struct Work {
  uv_work_t  request;
  uv_mutex_t mutex;
  uv_cond_t cond;
  Persistent<Function> callback;

  int value;
  int delay;
};

static void data_is_ready(unsigned long received_value, unsigned int received_delay, unsigned int received_protocol, unsigned int received_bit_length, void *userdata) {
	Work *work = static_cast<Work *>(userdata);
	work->value = received_value;
	work->delay = received_delay;

	uv_mutex_lock(&work->mutex);
	uv_cond_signal(&work->cond);
	uv_mutex_unlock(&work->mutex);
}

static void SnifferAsync(uv_work_t *req) {
	Work *work = static_cast<Work *>(req->data);
	Isolate * isolate = Isolate::GetCurrent();
	uv_mutex_lock(&work->mutex);
	uv_cond_wait(&work->cond, &work->mutex);
	uv_mutex_unlock(&work->mutex);
	
}
static void SnifferAsyncComplete(uv_work_t *req,int status) {
	Isolate * isolate = Isolate::GetCurrent();
	v8::HandleScope handleScope(isolate);

	//Local<Array> result_list = Array::New(isolate);
	Work *work = static_cast<Work *>(req->data);
	
	uv_cond_destroy(&work->cond);
	uv_mutex_destroy(&work->mutex);
	
	Local<Value> argv[] = {
		Integer::New(isolate, work->value),
		Integer::New(isolate, work->delay)
	};
	Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 2, argv);
	delete work;
}
void SetupSniffer(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	
	int pin = Handle<Integer>::Cast(args[0])->Int32Value();
	int tolerance = Handle<Integer>::Cast(args[1])->Int32Value();
	
	rcswitch_init();
	if(tolerance) {
		rcswitch_set_receive_tolerance(tolerance);
	}
	rcswitch_enable_receive(pin);
}

void Sniffer(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	if (!args[0]->IsFunction()) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate, "Argument must be a callback function")));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}

	Work * work = new Work();
	work->request.data = work;
	
	Local<Function> callback = Local<Function>::Cast(args[0]);
	work->callback.Reset(isolate, callback);
	
	uv_mutex_init(&work->mutex);
	uv_cond_init(&work->cond);
	
	uv_queue_work(uv_default_loop(),&work->request,SnifferAsync,SnifferAsyncComplete);
	rcswitch_set_data_ready_cb(data_is_ready, work);
	args.GetReturnValue().Set(Undefined(isolate));
}

void Send(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	int pin = Handle<Integer>::Cast(args[0])->Int32Value();
	unsigned int value = Handle<Integer>::Cast(args[1])->Uint32Value();
	unsigned int pulse = Handle<Integer>::Cast(args[2])->Uint32Value();

	rcswitch_send(1, pin, 10, pulse, value, 24);
	args.GetReturnValue().Set(Undefined(isolate));
}

void init(Handle <Object> exports, Handle<Object> module) {
	NODE_SET_METHOD(exports, "setupSniffer", SetupSniffer);
  NODE_SET_METHOD(exports, "sniffer", Sniffer);
  NODE_SET_METHOD(exports, "send", Send);
}

NODE_MODULE(modulename, init);
