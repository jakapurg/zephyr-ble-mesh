#include <gpio.h>
#include <bluetooth/bluetooth.h>
#include <settings/settings.h>
#include <bluetooth/mesh.h>
#define LED0 LED0_GPIO_PIN

u16_t reply_addr;
u8_t reply_net_idx;
u8_t reply_app_idx;

static const uint8_t DEVICE_UUID[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x01};
struct device *dev;
u8_t current_state;
void write_LED_state(uint8_t state){
    gpio_pin_write(dev,LED0,state);
}

static const struct bt_mesh_health_srv_cb health_srv_callback = {
};

//returning int because .output_number expects int
static int show_provisioning_pin(bt_mesh_output_action_t action, u32_t number){
    printk("PIN: %u\n", number);
    return 0;
}
static void provisioning_completed(u16_t net_idx, u16_t addr){
    printk("Provisioning completed\n");
}
static void provisioning_reset(void){
    //begin advertising again
    bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static bool serverHasAddress(struct bt_mesh_model *model){
    return model->pub->addr == BT_MESH_ADDR_UNASSIGNED;
}

//provisioning properties and capabilities
static const struct bt_mesh_prov prov = {
    .uuid = DEVICE_UUID,
    .output_size = 4, //4 digit PIN
    .output_actions = BT_MESH_DISPLAY_NUMBER,
    .output_number = show_provisioning_pin,
    .complete = provisioning_completed,
    .reset = provisioning_reset
};
void send_or_publish_status(bool publish, u8_t state);
static void set_light_state(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf, bool ack){
    uint8_t state = net_buf_simple_pull_u8(buf);
    if(state == current_state){
        //State is the same as it was, nothing should happen
        return;
    }
    current_state = state;
    //if state equals 0 turn off, else turn light on
    if(current_state){
        write_LED_state(0);
    }
    else{
        write_LED_state(1);
    }
    //If ack is true, send status back to switch
    if(ack){
        send_or_publish_status(false, current_state);
    }
    //If a server has a public address, it gets status on state change
    if(!serverHasAddress(model)){
        send_or_publish_status(true,current_state);
    }
}
static void get_state(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf){
    printk("Acknowledged GET\n");
    reply_addr = ctx->addr;
    reply_net_idx = ctx->net_idx;
    reply_app_idx = ctx->app_idx;
    send_or_publish_status(false,current_state);
}

static void set_state(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf){
    printk("SET state\n");
    set_light_state(model,ctx,buf,true);
}

static void set_state_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf){
    printk("Unacknowledged SET state\n");
    set_light_state(model,ctx,buf,false);
}

//define a model publication context
//parameter 1: context name, parameter 2: message update callback, parameter 3: message length
BT_MESH_MODEL_PUB_DEFINE(light_pub, NULL, 2+1);



//declare what should happen on selected operating code
static const struct bt_mesh_model_op onoff_opcodes[] = {
    {BT_MESH_MODEL_OP_2(0x82,0x01), 0, get_state},
    {BT_MESH_MODEL_OP_2(0x82,0x02), 2, set_state},
    {BT_MESH_MODEL_OP_2(0x82, 0x03), 2, set_state_unack},
    BT_MESH_MODEL_OP_END
};



//each node should support configuration server model
//indicate our server can act as proxy so we can provision with an app

static struct bt_mesh_cfg_srv cfg_srv = {
    .relay = BT_MESH_RELAY_DISABLED,
    .beacon = BT_MESH_BEACON_DISABLED,
    .frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
    .gatt_proxy = BT_MESH_GATT_PROXY_ENABLED,
    .default_ttl = 7,
    .net_transmit = BT_MESH_TRANSMIT(2,20) //every message is transmitted 3 times with intervals at max 20ms
};

//each node should support health server model
BT_MESH_HEALTH_PUB_DEFINE(health_pub,0);
static struct bt_mesh_health_srv health_srv = {
    .cb = &health_srv_callback
};

//list models: configuration model, health server model & model for turning light
static struct bt_mesh_model models[] = {
    BT_MESH_MODEL_CFG_SRV(&cfg_srv),
    BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, onoff_opcodes, &light_pub, NULL)
};

//define elements that have previously defined models
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, models, BT_MESH_MODEL_NONE)
};

//node definition
static const struct bt_mesh_comp composition = {
    .cid = 0xFFFF, //company ID for testing
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements)
};

void send_or_publish_status(bool publish, u8_t state){
    int err;
    struct bt_mesh_model *model = &models[2];
    if(publish && !serverHasAddress(model)){
        printk("No publish address!\n");
        return;
    }

    if(publish){
        //if publish is requested get message
        struct net_buf_simple *msg = model->pub->msg;
        net_buf_simple_reset(msg);
        //add opcode
        bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82,0x04));
        //add state
        net_buf_simple_add_u8(msg,state);
        //publish
        err = bt_mesh_model_publish(model);
        if(err){
            printk("Published failed with error %d\n", err);
        }
    }
    else{
        //prepare buffer
        NET_BUF_SIMPLE_DEFINE(msg, 7);
        bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_2(0x82,0x04));
        net_buf_simple_add_u8(&msg, state);
        //declare address (source)
        struct bt_mesh_msg_ctx ctx = {
            .net_idx = reply_net_idx,
            .app_idx = reply_app_idx,
            .addr = reply_addr,
            .send_ttl = BT_MESH_TTL_DEFAULT
        };
        //send prepared message
        err = bt_mesh_model_send(model, &ctx, &msg, NULL, NULL);
        if(err){
            printk("Sending failed with error %d\n", err);
        }

    }
}
static void bt_callback(int err){
    //bt_mesh_init returns 0 on success, 
    //parameter 1: node provisioning information, parameter 2: node composition
    err = bt_mesh_init(&prov, &composition);
    if(err){
        printk("Bluetooth Mesh initialization failed with error %d\n",err);
        return;
    }
    printk("Bluetooth Mesh initialized \n");

    //check if device has already been provisioned
    if(IS_ENABLED(CONFIG_SETTINGS)){
        //restore netkey, application key, SEQ field, other mesh stack variables
        settings_load();
    }

    if(!bt_mesh_is_provisioned()){
        //start advertising device
        bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
    }
}

void configure_light(){
    dev = device_get_binding(LED0_GPIO_CONTROLLER);
    gpio_pin_configure(dev, LED0, GPIO_DIR_OUT);
}

void main(){
    configure_light();
    //bt_enable returns 0 on success, argument is callback to notify completion
    int bt_err = bt_enable(bt_callback);
    if(bt_err){
        printk("enabling bluetooth failed with error %d\n", bt_err);
    }
}