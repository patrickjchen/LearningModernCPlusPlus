class H2O {
public:
    H2O(): OTicket { 2 }, HTicket { 2 } {
        
    }

    std::atomic<int> OTicket;
    std::atomic<int> HTicket;

    void hydrogen(function<void()> releaseHydrogen) {
        int H = 0;
        do {
          H = HTicket.load();
          while (H < 1) {
            HTicket.wait(H);
            H = HTicket.load();
          }
        } 
        while (H <= 0 || !HTicket.compare_exchange_strong(H, H - 1));

        releaseHydrogen();

        OTicket.fetch_add(1);
        OTicket.notify_one();
    }

    void oxygen(function<void()> releaseOxygen) {
        int O = 0;
        do {
          O = OTicket.load();
          while (O < 2) {
            OTicket.wait(O);
            O = OTicket.load();
          }
        } while (!OTicket.compare_exchange_strong(O, O - 2));

        releaseOxygen();
        
        HTicket.fetch_add(2);
        HTicket.notify_one();
        HTicket.notify_one();
    }
};
