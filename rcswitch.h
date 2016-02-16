#ifdef __cplusplus
extern "C" {
#endif

#define RCSWITCH_MAX_CHANGES 67
typedef void (*data_ready_t)(unsigned long received_value, unsigned int received_delay, unsigned int received_protocol, unsigned int received_bit_length, void *userdata);

void rcswitch_init();
void rcswitch_set_data_ready_cb(data_ready_t cb, void *data);
void rcswitch_set_receive_tolerance(int percent);
void rcswitch_set_protocol(int protocol);
int rcswitch_enable_receive(int interrupt);

void rcswitch_send(int protocol, int pin, int repeat, int pulse, unsigned long code, unsigned int length);
void rcswitch_send_scode_word(int protocol, int pin, int repeat, int pulse, char* sCodeWord);


#ifdef __cplusplus
}
#endif