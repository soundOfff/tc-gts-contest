# tc-gts-contest
<<<<<<< HEAD
This repo is for the contest of the GTS sponsor
=======

This repo is for the contest of the GST sponsor

** key concepts **
- Constant limit of total spot (ex. 10.000.000)
- CCY1 / CCY2 (pair of money) ex. Eur/Us Dollar
- API Socket (ms)
- Timestamps in nanoseconds, postEvent(), askPrice & askSize buy price. bidSize & bidPrice ?.
- side -> buy/sell || TIF -> GTC (Good till cancel) | IOC (Immediate or cancel)
- OrderStateObserver -> Receive updates
- OrderSender -> interface for sending orders

** key fn **
- sendOrder -> sends an order and returns an order ID.
- onAck, onFill, onTerminated -> callback for order updates.

** strategy class **
- Interface for implement the strategy

>>>>>>> c529668 (chore: fix readme)
