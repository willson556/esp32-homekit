#pragma once

#include <string>
#include <tuple>
#include <vector>
#include <functional>
#include <math.h>

#include "hap.h"

namespace HAP
{

class Accessory;

class Characteristic
{
public:
    /**
     * @brief Call to trigger a change notification outside of a write.
     * 
     */
    void notify()
    {
        if (!canRead())
        {
            return;
        }

        value_changed(read());
    }

    /**
     * @brief Can be used by other components to monitor the value of the characteristic.
     * 
     * @param callback function to be called when the value changes.
     */
    void register_for_notifications(std::function<void(Characteristic*)> callback)
    {
        value_changed_listeners.push_back(callback);
    }

protected:
    /**
     * @brief Construct a new Characteristic object.
     * 
     * @param type type of characteristic
     */
    Characteristic(hap_characteristic_type type)
        : type{type}
    {
    }

    /**
     * @brief Should be called whenever the value changes.
     *        Typically called in the write() method.
     * 
     * @param new_value updated value
     */
    void value_changed(void* new_value)
    {
        if (event_handle)
        {
            hap_event_response(accessory_handle, event_handle, new_value);
        }

        for (auto &listener : value_changed_listeners)
        {
            if (listener) listener(this);
        }
    }

    /**
     * @brief Gets the current value of the characteristic.
     * 
     * @return void* value
     */
    virtual void* read() const = 0;

    /**
     * @brief Gets a value indicating whether this characteristic supports reads.
     * 
     * @return true if the characteristic supports reads
     * @return false if the characteristic does not support reads
     */
    virtual bool canRead() const = 0;

    /**
     * @brief Writes a new value to the characteristic.
     * 
     * @param value new value
     * @param len length of the value
     */
    virtual void write(void *value, size_t len) = 0;

    /**
     * @brief Gets a value indicating whether this characteristic supports writes.
     * 
     * @return true if the characteristic supports writes
     * @return false if the characteristic does not support writes
     */
    virtual bool canWrite() const = 0;

    /**
     * @brief Gets max value information.
     * 
     * @return std::tuple<bool,void*> If the first element is true, the second element should be the max value.
     *                                Otherwise, the default max value will be used.
     */
    virtual std::tuple<bool,void*> get_max_value_override() const
        { return std::make_tuple(false, nullptr); }

    /**
     * @brief Gets min value information.
     * 
     * @return std::tuple<bool,void*> If the first element is true, the second element should be the min value.
     *                                Otherwise, the default min value will be used.
     */
    virtual std::tuple<bool,void*> get_min_value_override() const
        { return std::make_tuple(false, nullptr); }

    /**
     * @brief Get the valid values information.
     * 
     * @return std::tuple<bool,std::vector<int>> If the first element is true, the second element should be a list of valid values.
     *                                           Otherwise, the default values will be used.
     */
    virtual std::tuple<bool,std::vector<int>> get_valid_values_override() const
        { return std::make_tuple(false, std::vector<int>{}); }

    friend void* read_characteristic(void *arg);
    friend void write_characteristic(void *arg, void *value, int len);
    friend void set_characteristic_event_handle(void *arg, void *event_handle, bool enable);

    friend Accessory; // Needs to set accessory_handle.

private:
    void set_event_handle(void *handle, bool enable)
    {
        event_handle = enable ? handle : nullptr;
    }

    hap_characteristic_ex get_characteristic_struct() const;
    static void delete_characteristic_struct_internals(hap_characteristic_ex &cs);
    std::vector<std::function<void(Characteristic*)>> value_changed_listeners {};

    const hap_characteristic_type type;
    void* accessory_handle;
    void* event_handle{nullptr};
};

/**
 * @brief Mix-in class for taking a functional approach to the characteristics.
 * 
 * @tparam T type of value being read and written.
 */
template <typename T>
class FunctionCharacteristic
{
public:
    /**
     * @brief Construct a new Function Characteristic object.
     * 
     * @param readFunction function to call when the characteristic is read
     * @param writeFunction function to call when the characteristic is written to.
     */
    FunctionCharacteristic(std::function<T()> readFunction,
                           std::function<void(T)> writeFunction)
        : readFunction{readFunction},
          writeFunction{writeFunction}
    {
    }

protected:
    const std::function<T()> readFunction;
    const std::function<void(T)> writeFunction;
};

/**
 * @brief String Characteristic
 * 
 */
class StringCharacteristic : public Characteristic
{
protected:

    StringCharacteristic(hap_characteristic_type type)
        : Characteristic{type}
    {
    }

    virtual std::string readString() const = 0;
    virtual void writeString(std::string) = 0;

    void* read() const override
    {
        return (void*)(readString().c_str());
    }

    void write(void *value, size_t len) override
    {
        writeString({ (char*)value });
        value_changed(value);
    }
};

/**
 * @brief Functional String Characteristic
 * 
 */
class StringFunctionCharacteristic : public StringCharacteristic, public FunctionCharacteristic<std::string>
{
public:
    StringFunctionCharacteristic(hap_characteristic_type type,
                                 std::function<std::string()> readFunction,
                                 std::function<void(std::string)> writeFunction)
        : StringCharacteristic{type},
          FunctionCharacteristic{readFunction, writeFunction}
    {
    }

protected:
    std::string readString() const override { return readFunction(); }
    void writeString(std::string value) override { writeFunction(value); }
    
    bool canRead() const override { return readFunction != nullptr; }
    bool canWrite() const override { return writeFunction != nullptr; }
};

/**
 * @brief Float Characteristic
 * 
 */
class FloatCharacteristic : public Characteristic
{
protected:
    FloatCharacteristic(hap_characteristic_type type)
        : Characteristic{type}
    {
    }

    virtual float readFloat() const = 0;
    virtual void writeFloat(float) = 0;

    void* read() const override
    {
        return floatToVoid(readFloat());
    }

    void write(void *value, size_t len) override
    {
        writeFloat(voidToFloat(value));
        value_changed(value);
    }

    virtual std::tuple<bool,float> get_max_value_override_float() const
        { return std::make_tuple(false, 0.0f); }
    virtual std::tuple<bool,float> get_min_value_override_float() const
        { return std::make_tuple(false, 0.0f); }

    virtual std::tuple<bool,void*> get_max_value_override() const
    {
        auto result = get_max_value_override_float();
        return std::make_tuple(std::get<0>(result),
                               floatToVoid(std::get<1>(result)));
    }
    virtual std::tuple<bool,void*> get_min_value_override() const
    {
        auto result = get_min_value_override_float();
        return std::make_tuple(std::get<0>(result),
                               floatToVoid(std::get<1>(result)));
    }

    std::tuple<bool,std::vector<int>> get_valid_values_override() const
        { return std::make_tuple(false, std::vector<int>{}); }
private:
    void* floatToVoid(float value) const
    {
        auto intValue {static_cast<int>(round(value * 100.0f))};
        return reinterpret_cast<void*>(intValue);
    }

    float voidToFloat(void* value) const
    {
        auto intValue = reinterpret_cast<int>(value);
        return intValue / 100.0f;
    }
};

/**
 * @brief Functional Float Characteristic
 * 
 */
class FloatFunctionCharacteristic : public FloatCharacteristic, public FunctionCharacteristic<float>
{
public:
    FloatFunctionCharacteristic(hap_characteristic_type type,
                                std::function<float()> readFunction,
                                std::function<void(float)> writeFunction)
        : FloatCharacteristic{type},
          FunctionCharacteristic{readFunction, writeFunction}
    {
    }

protected:
    float readFloat() const override { return readFunction(); }
    void writeFloat(float value) override { writeFunction(value); }

    bool canRead() const override { return readFunction != nullptr; }
    bool canWrite() const override { return writeFunction != nullptr; }
};

/**
 * @brief Int Characteristic
 * 
 */
class IntCharacteristic : public Characteristic
{
protected:
    IntCharacteristic(hap_characteristic_type type)
        : Characteristic{type}
    {
    }

    virtual int readInt() const = 0;
    virtual void writeInt(int) = 0;

    void* read() const override
    {
        return reinterpret_cast<void*>(readInt());
    }

    void write(void *value, size_t len) override
    {
        writeInt(reinterpret_cast<int>(value));
        value_changed(value);
    }

    virtual std::tuple<bool,int> get_max_value_override_int() const
        { return std::make_tuple(false, 0); }
    virtual std::tuple<bool,int> get_min_value_override_int() const
        { return std::make_tuple(false, 0); }

    virtual std::tuple<bool,void*> get_max_value_override() const
    {
        auto result = get_max_value_override_int();
        return std::make_tuple(std::get<0>(result),
                               reinterpret_cast<void*>(std::get<1>(result)));
    }

    virtual std::tuple<bool,void*> get_min_value_override() const
    {
        auto result = get_min_value_override_int();
        return std::make_tuple(std::get<0>(result),
                               reinterpret_cast<void*>(std::get<1>(result)));
    }
};

/**
 * @brief Functional Int Characteristic
 * 
 */
class IntFunctionCharacteristic : public IntCharacteristic, public FunctionCharacteristic<int>
{
public:
    IntFunctionCharacteristic(hap_characteristic_type type,
                                 std::function<int()> readFunction,
                                 std::function<void(int)> writeFunction)
        : IntCharacteristic{type},
          FunctionCharacteristic{readFunction, writeFunction}
    {
    }

protected:
    int readInt() const override { return readFunction(); }
    void writeInt(int value) override { writeFunction(value); }

    bool canRead() const override { return readFunction != nullptr; }
    bool canWrite() const override { return writeFunction != nullptr; }
};

void accessory_init(void* arg);

/**
 * @brief Homekit Accessory Protocol Accessory
 * 
 */
class Accessory
{
public:
    /**
     * @brief Call to initialize and register the accessory.
     * 
     */
    void register_accessory();

protected:
    /**
     * @brief Construct a new Accessory object.
     * 
     * @param name name of the device
     * @param id device id
     * @param setup_code setup code
     * @param manufacturer_name manufacturer name
     * @param firmware_version firmware version
     * @param model device model
     * @param serial_number device serial number
     * @param category device category
     * @param port commmunication port
     * @param configuration_version configuration version (increment when the characteristics change)
     */
    Accessory(const std::string &name,
                 const std::string &id,
                 const std::string &setup_code,
                 const std::string &manufacturer_name,
                 const std::string &firmware_version,
                 const std::string &model,
                 const std::string &serial_number,
                 hap_accessory_category category,
                 int port,
                 int configuration_version)
            : name{name},
              id{id},
              setup_code{setup_code},
              manufacturer_name{manufacturer_name},
              firmware_version{firmware_version},
              model{model},
              serial_number{serial_number},
              category{category},
              port{port},
              configuration_version{configuration_version}
    {
    }

    void add_service(hap_service_type service_type,
                     const std::vector<Characteristic*> &characteristics);

    virtual void init() = 0;

    friend void accessory_init(void* arg);

private:
    void init_callback();

    const std::string &name;
    const std::string &id;
    const std::string &setup_code;
    const std::string &manufacturer_name;
    const std::string &firmware_version;
    const std::string &model;
    const std::string &serial_number;
    const hap_accessory_category category;
    const int port;
    const int configuration_version;

    void* accessory_handle{nullptr};
    void* accessory_object{nullptr};

    hap_accessory_callback_t callback {accessory_init};

    static bool hap_initialized;
};

} // namespace HAP