#include <gpio.h>

#include <bluetooth/bluetooth.h>

#include <settings/settings.h>

#include <bluetooth/mesh.h>

#ifdef SW0_GPIO_FLAGS
#define EDGE(SW0_GPIO_FLAGS | GPIO_INT_EDGE)
#else
#define EDGE(GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW)
#endif
#ifndef SW0_GPIO_FLAGS
#ifdef SW0_GPIO_PIN_PUD
#define SW0_GPIO_FLAGS SW0_GPIO_PIN_PUD
#else
#define SW0_GPIO_FLAGS 0
#endif
#endif
#define PULL_UP SW0_GPIO_FLAGS

static
const uint8_t DEVICE_UUID[16] = {
    0x00,
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0A,
    0x0B,
    0x0C,
    0x0D,
    0x0E,
    0x00
};
static u8_t ON = 1;
static u8_t OFF = 0;
static u8_t tid = 0;
static struct gpio_callback btn1_cb;
static struct gpio_callback btn2_cb;
static struct gpio_callback btn3_cb;
static struct gpio_callback btn4_cb;

//to work with k_work_submit, so we can handle button pressing in a background thread
static struct k_work btn1_work;
static struct k_work btn2_work;
static struct k_work btn3_work;
static struct k_work btn4_work;

//each node should support configuration server model
//indicate our server can act as proxy so we can provision with an app

static struct bt_mesh_cfg_srv cfg_srv = {
    .relay = BT_MESH_RELAY_DISABLED,
    .beacon = BT_MESH_BEACON_DISABLED,
    .frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
    .gatt_proxy = BT_MESH_GATT_PROXY_ENABLED,
    .default_ttl = 7,
    .net_transmit = BT_MESH_TRANSMIT(2, 20) //every message is transmitted 3 times with intervals at max 20ms
};

//returning int because .output_number expects int
static int show_provisioning_pin(bt_mesh_output_action_t action, u32_t number) {
    printk("PIN: %u\n", number);
    return 0;
}

static bool serverHasAddress(struct bt_mesh_model * model) {
    return model -> pub -> addr == BT_MESH_ADDR_UNASSIGNED;
}

static void provisioning_completed(u16_t net_idx, u16_t addr) {
    printk("Provisioning completed\n");
}

static void provisioning_reset(void) {
    //begin advertising again
    bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

//provisioning properties and capabilities
static
const struct bt_mesh_prov prov = {
    .uuid = DEVICE_UUID,
    .output_size = 4, //4 digit PIN
    .output_actions = BT_MESH_DISPLAY_NUMBER,
    .output_number = show_provisioning_pin,
    .complete = provisioning_completed,
    .reset = provisioning_reset
};

//each node should support health server model
BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static
const struct bt_mesh_health_srv_cb health_srv_callback = {};

static struct bt_mesh_health_srv health_srv = {
    .cb = & health_srv_callback
};

static void print_state(struct bt_mesh_model * model, struct bt_mesh_msg_ctx * ctx, struct net_buf_simple * buf) {
    u8_t state = net_buf_simple_pull_u8(buf);
    //print received status
    printk("State: %d\n", state);
}
//declare what should happen on selected operating code
static
const struct bt_mesh_model_op opcodes[] = {
    {
        BT_MESH_MODEL_OP_2(0x82, 0x04), 1, print_state
    },
    BT_MESH_MODEL_OP_END
};

//define a model publication context
//parameter 1: context name, parameter 2: message update callback, parameter 3: message length
BT_MESH_MODEL_PUB_DEFINE(switch_pub, NULL, 2);

static struct bt_mesh_model models[] = {
    BT_MESH_MODEL_CFG_SRV( & cfg_srv),
    BT_MESH_MODEL_HEALTH_SRV( & health_srv, & health_pub),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, opcodes, & switch_pub, & OFF)
};

//define elements that have previously defined models
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, models, BT_MESH_MODEL_NONE)
};

//node definition
static
const struct bt_mesh_comp composition = {
    .cid = 0xFFFF, //company ID for testing
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements)
};

//state -> value 0 or 1
//message_type -> opcode that tells if we expect acknowledgment
int send_state(u8_t state, u16_t message_type) {
    struct bt_mesh_model * model = & models[2];
    if (!serverHasAddress(model)) {
        printk("No publish address\n");
        return -1;
    }
    struct net_buf_simple * msg = model -> pub -> msg;
    //added opcode to message
    bt_mesh_model_msg_init(msg, message_type);
    //add state
    net_buf_simple_add_u8(msg, state);
    //add TID (transaction identifier)
    net_buf_simple_add_u8(msg, tid);
    tid++;
    //publish message
    int err = bt_mesh_model_publish(model);
    if (err) {
        printk("Published failed with error %d\n", err);
    }
    return err;
}

int get_state() {
    struct bt_mesh_model * model = & models[2];
    if (!serverHasAddress(model)) {
        printk("No publish address\n");
        return -1;
    }
    struct net_buf_simple * msg = model -> pub -> msg;
    //add opcode
    bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x01));
    //publish
    int err = bt_mesh_model_publish(model);
    if (err) {
        printk("Published failed with error %d\n", err);
    }
    return err;
}

void set_state(u8_t state) {
    int err = send_state(state, BT_MESH_MODEL_OP_2(0x82, 0x02));
    if (err) {
        printk("Sending state failed with error %d\n", err);
    }
}

//Handle for pressing Button1
void btn1_handle(struct k_work * work) {
    set_state(ON);
}

//Handle for pressing Button2
void btn2_handle(struct k_work * work) {
    set_state(OFF);
}

//Handle for pressing Button3
void btn3_handle(struct k_work * work) {
    get_state();
}

//Handle for pressing Button4
void btn4_handle(struct k_work * work) {

}

void btn1_pressed(struct device * gpiob, struct gpio_callback * cb, u32_t pin) {
    k_work_submit( & btn1_work);
}
void btn2_pressed(struct device * gpiob, struct gpio_callback * cb, u32_t pin) {
    k_work_submit( & btn2_work);
}
void btn3_pressed(struct device * gpiob, struct gpio_callback * cb, u32_t pin) {
    k_work_submit( & btn1_work);
}
void btn4_pressed(struct device * gpiob, struct gpio_callback * cb, u32_t pin) {
    k_work_submit( & btn4_work);
}
void configure_buttons() {
    struct device * port_btn1;
    struct device * port_btn2;
    struct device * port_btn3;
    struct device * port_btn4;
    //bind ports for buttons
    port_btn1 = device_get_binding(SW0_GPIO_CONTROLLER);
    if (!port_btn1) {
        printk("Obtaining port for BTN1 failed\n");
        return;
    }

    port_btn2 = device_get_binding(SW1_GPIO_CONTROLLER);
    if (!port_btn2) {
        printk("Obtaining port for BTN2 failed\n");
        return;
    }
    port_btn3 = device_get_binding(SW2_GPIO_CONTROLLER);
    if (!port_btn3) {
        printk("Obtaining port for BTN3 failed\n");
        return;
    }

    port_btn4 = device_get_binding(SW3_GPIO_CONTROLLER);
    if (!port_btn4) {
        printk("Obtaining port for BTN4 failed\n");
        return;
    }

    //Button 1
    k_work_init( & btn1_work, btn1_handle);
    gpio_pin_configure(port_btn1, SW0_GPIO_PIN, GPIO_DIR_IN | GPIO_INT | PULL_UP | EDGE);
    gpio_init_callback( & btn1_cb, btn1_pressed, BIT(SW0_GPIO_PIN));
    gpio_add_callback(port_btn1, & btn1_cb);
    gpio_pin_enable_callback(port_btn1, SW0_GPIO_PIN);

    //Button 2
    k_work_init( & btn2_work, btn2_handle);
    gpio_pin_configure(port_btn2, SW1_GPIO_PIN, GPIO_DIR_IN | GPIO_INT | PULL_UP | EDGE);
    gpio_init_callback( & btn2_cb, btn2_pressed, BIT(SW1_GPIO_PIN));
    gpio_add_callback(port_btn2, & btn2_cb);
    gpio_pin_enable_callback(port_btn2, SW1_GPIO_PIN);

    //Button 3
    k_work_init( & btn3_work, btn3_handle);
    gpio_pin_configure(port_btn3, SW2_GPIO_PIN, GPIO_DIR_IN | GPIO_INT | PULL_UP | EDGE);
    gpio_init_callback( & btn3_cb, btn3_pressed, BIT(SW2_GPIO_PIN));
    gpio_add_callback(port_btn3, & btn3_cb);
    gpio_pin_enable_callback(port_btn3, SW2_GPIO_PIN);

    //Button 4
    k_work_init( & btn4_work, btn4_handle);
    gpio_pin_configure(port_btn4, SW3_GPIO_PIN, GPIO_DIR_IN | GPIO_INT | PULL_UP | EDGE);
    gpio_init_callback( & btn4_cb, btn4_pressed, BIT(SW3_GPIO_PIN));
    gpio_add_callback(port_btn4, & btn4_cb);
    gpio_pin_enable_callback(port_btn4, SW3_GPIO_PIN);

}

static void bt_callback(int err) {
    //bt_mesh_init returns 0 on success, 
    //parameter 1: node provisioning information, parameter 2: node composition
    err = bt_mesh_init( & prov, & composition);
    if (err) {
        printk("Bluetooth Mesh initialization failed with error %d\n", err);
        return;
    }
    printk("Bluetooth Mesh initialized \n");

    //check if device has already been provisioned
    if (IS_ENABLED(CONFIG_SETTINGS)) {
        //restore netkey, application key, SEQ field, other mesh stack variables
        settings_load();
    }

    if (!bt_mesh_is_provisioned()) {
        //start advertising device
        bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
    }
}

void main() {
    configure_buttons();
    int bt_err = bt_enable(bt_callback);
    if (bt_err) {
        printk("enabling bluetooth failed with error %d\n", bt_err);
    }

}