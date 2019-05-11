#include "hap.hpp"

#include <cstring>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

namespace HAP
{

bool Accessory::hap_initialized {false};

void *identify_read(void *arg)
{
    return (void *)true;
}

void* read_characteristic(void *arg)
{
    return static_cast<Characteristic*>(arg)->read();
}

void write_characteristic(void *arg, void *value, int len)
{
    static_cast<Characteristic*>(arg)->write(value, static_cast<size_t>(len));
}

void set_characteristic_event_handle(void *arg, void *event_handle, bool enable)
{
    static_cast<Characteristic*>(arg)->set_event_handle(event_handle, enable);
}

void accessory_init(void *arg)
{
    static_cast<Accessory*>(arg)->init_callback();
}

hap_characteristic_ex Characteristic::get_characteristic_struct() const
{
    void *max_value, *min_value;
    std::vector<int> valid_values;
    bool override_max_value, override_min_value, override_valid_values;

    std::tie(override_max_value, max_value) = get_max_value_override();
    std::tie(override_min_value, min_value) = get_min_value_override();
    std::tie(override_valid_values, valid_values) = get_valid_values_override();

    int *valid_values_dyn = nullptr;
    if (override_valid_values)
    {
        valid_values_dyn = new int[valid_values.size()];
        memcpy(valid_values_dyn, valid_values.data(), valid_values.size() * sizeof(int));
    }

    return
    {
        type,
        read(),
        (void*)this,
        canRead() ? read_characteristic : nullptr,
        canWrite() ? write_characteristic : nullptr,
        set_characteristic_event_handle,
        override_max_value,
        max_value,
        override_min_value,
        min_value,
        false,
        nullptr,
        override_valid_values,
        override_valid_values ? valid_values.size() : 0,
        valid_values_dyn,
    };
}

void Characteristic::delete_characteristic_struct_internals(hap_characteristic_ex &cs)
{
    if (cs.override_valid_values && cs.valid_values)
    {
        delete[] cs.valid_values;
    }
}

void Accessory::register_accessory()
{
    if (!hap_initialized)
    {
        hap_initialized = true;
        hap_init();
    }

    accessory_handle = hap_accessory_register(
        name.c_str(),
        id.c_str(),
        setup_code.c_str(),
        manufacturer_name.c_str(),
        category,
        port,
        configuration_version,
        this,
        &callback
    );
}

void Accessory::init_callback()
{
    accessory_object = hap_accessory_add(accessory_handle);

    struct hap_characteristic cs[] = {
        {HAP_CHARACTER_IDENTIFY, (void *)true, NULL, identify_read, NULL, NULL},
        {HAP_CHARACTER_MANUFACTURER, (void *)manufacturer_name.c_str(), NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_MODEL, (void *)model.c_str(), NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_NAME, (void *)name.c_str(), NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_SERIAL_NUMBER, (void *)serial_number.c_str(), NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_FIRMWARE_REVISION, (void *)firmware_version.c_str(), NULL, NULL, NULL, NULL},
    };

    hap_service_and_characteristics_add(accessory_handle,
                                        accessory_object,
                                        HAP_SERVICE_ACCESSORY_INFORMATION,
                                        cs,
                                        ARRAY_SIZE(cs));

    init();
}

void Accessory::add_service(hap_service_type service_type,
                            const std::vector<Characteristic*> &characteristics)
{
    auto hap_cs = new hap_characteristic_ex[characteristics.size()];

    auto i = 0;
    for (auto characteristic: characteristics)
    {
        characteristic->accessory_handle = accessory_handle;
        hap_cs[i++] = characteristic->get_characteristic_struct();
    }

    hap_service_and_characteristics_ex_add(accessory_handle,
                                           accessory_object, 
                                           service_type,
                                           hap_cs,
                                           i);

    for (auto j = 0; j < i; ++j)
    {
        Characteristic::delete_characteristic_struct_internals(hap_cs[j]);
    }

    delete[] hap_cs;
}

} // namespace HAP