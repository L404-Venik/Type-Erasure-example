#include <iostream>
#include <memory>
#include <typeinfo>
#include <type_traits>
#include <stdexcept>

// класс для выдачи ошибки
class BadAnyCast : public std::bad_cast
{
public:
    const char* what() const noexcept override
    {
        return "Bad any cast";
    }
};

// класс для хранения переменной любого типа, реализующий идиому Type Erasure
class Any 
{
    struct BaseHolder
    {
        virtual ~BaseHolder() = default;
        virtual const std::type_info& type() const noexcept = 0;
        virtual std::unique_ptr<BaseHolder> clone() const = 0; // Добавляем виртуальный метод clone(), для копирования содержимого
    };

    template <typename T>
    struct Holder : BaseHolder 
    {
        T value;

        Holder(const T& v) : value(v) {}

        Holder(T&& v) : value(std::forward<T>(v)) {}

        const std::type_info& type() const noexcept override
        {
            return typeid(T);
        }

        std::unique_ptr<BaseHolder> clone() const override // Реализация метода clone()
        { 
            return std::make_unique<Holder<T>>(value);
        }
    };

    std::unique_ptr<BaseHolder> holder;
    
public:
    // конструкторы
    Any() = default;

    template <typename T>
    Any(T&& value)
        : holder(std::make_unique<Holder<std::decay_t<T>>>(std::forward<T>(value))) {}

    Any(const Any& other)
        : holder(other.holder ? other.holder->clone() : nullptr) {}

    Any(Any&&) noexcept = default;
    Any& operator=(const Any& other)
    {
        if (&other != this)
            holder = other.holder ? other.holder->clone() : nullptr;
        
        return *this;
    }

    Any& operator=(Any&&) noexcept = default;

    template <typename T>
    T& get() 
    {
        if (!holder || holder->type() != typeid(T))
            throw BadAnyCast();

        return static_cast<Holder<T>*>(holder.get())->value;
    }

    // методы
    template <typename T>
    const T& get() const
    {
        if (!holder || holder->type() != typeid(T))
            throw BadAnyCast();

        return static_cast<const Holder<T>*>(holder.get())->value;
    }

    const std::type_info& type() const noexcept
    {
        return holder ? holder->type() : typeid(void);
    }

    bool has_value() const noexcept
    {
        return holder != nullptr;
    }

    void reset() noexcept
    {
        holder.reset();
    }
};

int main()
{
    // примеры использования
    Any a(5);

    std::cout << a.get<int>() << std::endl;           // Успешно: выводит 5
    //std::cout << a.get<std::string>() << std::endl;   // Ошибка: исключение BadAnyCast

    Any b(1.5);
    std::cout << b.get<double>() << std::endl;

    b = std::string("working");
    std::cout << b.get<std::string>() << std::endl;

    b = a;
    std::cout << b.get<int>() << std::endl;
}