/*
 * QEMU NE2000 emulation -- isa bus windup
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "hw/hw.h"
#include "hw/pc.h"
#include "hw/isa.h"
#include "hw/qdev.h"
#include "net/net.h"
#include "hw/ne2000.h"
#include "exec/address-spaces.h"

typedef struct ISANE2000State {
    ISADevice dev;
    uint32_t iobase;
    uint32_t isairq;
    NE2000State ne2000;
} ISANE2000State;

static void isa_ne2000_cleanup(NetClientState *nc)
{
    NE2000State *s = qemu_get_nic_opaque(nc);

    s->nic = NULL;
}

static NetClientInfo net_ne2000_isa_info = {
    .type = NET_CLIENT_OPTIONS_KIND_NIC,
    .size = sizeof(NICState),
    .can_receive = ne2000_can_receive,
    .receive = ne2000_receive,
    .cleanup = isa_ne2000_cleanup,
};

static const VMStateDescription vmstate_isa_ne2000 = {
    .name = "ne2000",
    .version_id = 2,
    .minimum_version_id = 0,
    .minimum_version_id_old = 0,
    .fields      = (VMStateField []) {
        VMSTATE_STRUCT(ne2000, ISANE2000State, 0, vmstate_ne2000, NE2000State),
        VMSTATE_END_OF_LIST()
    }
};

static int isa_ne2000_initfn(ISADevice *dev)
{
    ISANE2000State *isa = DO_UPCAST(ISANE2000State, dev, dev);
    NE2000State *s = &isa->ne2000;

    ne2000_setup_io(s, 0x20);
    isa_register_ioport(dev, &s->io, isa->iobase);

    isa_init_irq(dev, &s->irq, isa->isairq);

    qemu_macaddr_default_if_unset(&s->c.macaddr);
    ne2000_reset(s);

    s->nic = qemu_new_nic(&net_ne2000_isa_info, &s->c,
                          object_get_typename(OBJECT(dev)), dev->qdev.id, s);
    qemu_format_nic_info_str(qemu_get_queue(s->nic), s->c.macaddr.a);

    return 0;
}

static Property ne2000_isa_properties[] = {
    DEFINE_PROP_HEX32("iobase", ISANE2000State, iobase, 0x300),
    DEFINE_PROP_UINT32("irq",   ISANE2000State, isairq, 9),
    DEFINE_NIC_PROPERTIES(ISANE2000State, ne2000.c),
    DEFINE_PROP_END_OF_LIST(),
};

static void isa_ne2000_class_initfn(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    ISADeviceClass *ic = ISA_DEVICE_CLASS(klass);
    ic->init = isa_ne2000_initfn;
    dc->props = ne2000_isa_properties;
}

static const TypeInfo ne2000_isa_info = {
    .name          = "ne2k_isa",
    .parent        = TYPE_ISA_DEVICE,
    .instance_size = sizeof(ISANE2000State),
    .class_init    = isa_ne2000_class_initfn,
};

static void ne2000_isa_register_types(void)
{
    type_register_static(&ne2000_isa_info);
}

type_init(ne2000_isa_register_types)
