class Foo {
    atomic<int> x = 0;
   
public:
    Foo() {
        
    }

    void first(function<void()> printFirst) {
        int current_value = x.load();
        while (current_value % 3 != 0) {
            x.wait(current_value);  // Efficiently waits until `x` changes
            current_value = x.load();
        }
        printFirst();
        x.fetch_add(1, std::memory_order_release);
        x.notify_all();
    }

    void second(function<void()> printSecond) {
        int current_value = x.load();
        while (current_value % 3 != 1) {
            x.wait(current_value);  // Efficiently waits until `x` changes
            current_value = x.load();
        }
        
        printSecond();
        x.fetch_add(1, std::memory_order_release);
        x.notify_all();     
    }

    void third(function<void()> printThird) {
        int current_value = x.load();
        while (current_value % 3 != 2) {
            x.wait(current_value);  // Efficiently waits until `x` changes
            current_value = x.load();
        }
        
        printThird();
        x.fetch_add(1, std::memory_order_release);
        x.notify_all();
    }
};

