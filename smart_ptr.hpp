#include <memory>
#include <utility>
#include <atomic>

namespace smart_pointer {
    class exception : std::exception {
        using base_class = std::exception;
        using base_class::base_class;
    };

// `SmartPointer` class declaration
    template<
            typename T
    >
    class SmartPointer {

    public:
        using value_type = T;

        explicit SmartPointer(value_type* ptr = nullptr) {
            if (ptr != nullptr)
                core = new Core(ptr);
            else
                core = nullptr;
        }

        // copy constructor
        SmartPointer(const SmartPointer& src) : core(src.core) {
            if (core != nullptr && core->ptr != nullptr)
                core->count++;
        }

        // move constructor
        SmartPointer(SmartPointer&& src) : core(std::move(src.core)) {
            src.core = nullptr;
        }

        // copy assigment
        SmartPointer& operator=(const SmartPointer& rhs) {
            tmp();
            if (rhs.core != nullptr)
                rhs.core->count++;
            core = rhs.core;
            return *this;
        }

        void tmp() {
            if (core != nullptr) {
                if (core->ptr != nullptr) {
                    core->count--;
                    if (core->count == 0) {
                        //core->alloc.deallocate(core->ptr, 1);
                        delete core->ptr;
                        core->ptr = nullptr;
                        delete core;
                        core = nullptr;
                    }
                } else {
                    delete core;
                    core = nullptr;
                }
            }
        }

        // move assigment
        SmartPointer& operator=(SmartPointer&& rhs) {
            tmp();

            this->core = std::move(rhs.core);
            rhs.core = nullptr;
            return *this;
        }

        //
        SmartPointer& operator=(value_type* rhs) {
            tmp();

            if (rhs != nullptr)
                core = new Core(rhs);
            else
                core = nullptr;
            return *this;
        }

        ~SmartPointer() {
            if (core != nullptr) {
                core->count--;
                if (core->count == 0) {
                    //core->alloc.deallocate(core->ptr, 1);
                    delete core->ptr;
                    core->ptr = nullptr;
                    delete core;
                    core = nullptr;
                }
            } else {
                delete core;
            }
        }

        // return reference to the object of class/type T
        // if SmartPointer contains nullptr throw `SmartPointer::exception`
        value_type& operator*() {
            if (core == nullptr || core->ptr == nullptr) {
                throw smart_pointer::exception();
            }

            return *(core->ptr);
        }
        const value_type& operator*() const {
            if (core == nullptr || core->ptr == nullptr) {
                throw smart_pointer::exception();
            }

            return *(core->ptr);
        }

        // return pointer to the object of class/type T
        value_type* operator->() const {
            if (core == nullptr)
                return nullptr;
            return core->ptr;
        }

        value_type* get() const {
            if (!core)
                return nullptr;
            return core->ptr;
        }

        // if pointer == nullptr => return false
        operator bool() const {
            return !(core == nullptr || core->ptr == nullptr);
        }

        // if pointers points to the same address or both null => true
        template<typename U>
        bool operator==(const SmartPointer<U>& rhs) const {
            if (!core && !rhs.get())
                return true;
            if (((core == nullptr) && (rhs.get() != nullptr)) ||
                ((core != nullptr) && (rhs.get() == nullptr)))
                return false;
            return (static_cast<void*>(core->ptr) ==
                    static_cast<void*>(rhs.get()));
        }


        // if pointers points to the same address or both null => false
        template<typename U>
        bool operator!=(const SmartPointer<U>& rhs) const {
            return !(*this == rhs);
        }

        // if smart pointer contains non-nullptr => return count owners
        // if smart pointer contains nullptr => return 0
        std::size_t count_owners() const {
            if ((core == nullptr) || (core->ptr == nullptr))
                return 0;
            return core->count;
        }

    private:
        class Core {
        public:
            explicit Core(T* ptr) : ptr(ptr) {
                if (this->ptr != nullptr)
                    count++;
            }

            T* ptr;
            //Allocator alloc;
            std::atomic<size_t> count = 0;
        };
        Core* core;
    };
}  // namespace smart_pointer