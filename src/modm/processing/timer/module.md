# Software Timers

This module provides polling based software timers for executing code after a
delay or periodically in millisecond resolution via `modm::Clock` and in
microsecond resolution via `modm::PreciseClock`.

To delay or delegate execution to the future, you can use `modm::Timeout` to set
a duration after which the timeout expires and executes your code:

```cpp
modm::Timeout timeout{100ms};
while (not timeout.isExpired()) ;
// your code after a delay
```

However, this construct is not very useful, particularly since you could also
simply use `modm::delay(100ms)` for this, so instead use the `execute()`
method to poll non-blockingly for expiration:

```cpp
modm::Timeout timeout{100ms};

void update()
{
    if (timeout.execute())
    {
        // your code after a expiration
    }
}
// You must call the update() function in your main loop now!
int main()
{
    while(1)
    {
        update();
    }
}
```

The `execute()` method returns true only once after expiration, so it can be
continuously polled somewhere in your code. A more comfortable use-case is to
use a `modm::Timeout` inside a class that needs to provide some asynchronous
method for timekeeping:

```cpp
class DelayEvents
{
    modm::Timeout timeout;
public:
    void event() { timeout.restart(100ms); }
    void update()
    {
        if (timeout.execute()) {
            // delegated code here
        }
    }
}
```

However, for more complex use-cases, these classes are intended to be used with
fibers (from module `modm:processing:fiber`) to implement non-blocking delays.

```cpp
modm::Fiber fiber_timeout([]
{
    modm::Timeout timeout(100ms);
    timeout.wait();
    // NOTE: for such simple delays, use the built-in fiber sleep function!
    modm::this_fiber::sleep_for(100ms);
});
```

For periodic timeouts, you could simply restart the timeout, however, the
`restart()` method schedules a timeout from the *current* time onwards:

```cpp
modm::Timeout timeout(100ms);
modm::Fiber fiber_timeout([]
{
    while(true)
    {
        timeout.wait(); // yields fiber until timeout
        // delayed code introduces latency
        timeout.restart(); // restarts but with *current* time!!!
    }
});
```

This can lead to longer period than required, particularly in a system that has
a lot to do and cannot service every timeout immediately. The solution is to use
a `modm::PeriodicTimer`, which only reimplements the `wait()` method to
automatically restart the timer, by adding the interval to the *old* time, thus
keeping the period accurate:

```cpp
modm::PeriodicTimer timer{100ms};
modm::Fiber fiber_timer([]
{
    while(true)
    {
        timer.wait(); // automatically restarts
        // code can take up to 100ms, but timer period won't drift
    }
});
```

The `wait()` method actually returns the number of missed periods, so that in a
heavily congested system you do not need to keep track of time yourself. This
can be particularly useful when dealing with soft real-time physical systems
like LED animations or control loops:

```cpp
modm::PeriodicTimer timer{1ms}; // render at 1kHz ideally
modm::Fiber fiber_timer([]
{
    while(true)
    {
        // call only once regardless of the number of periods
        animation.step(timer.wait()); // still compute the missing steps
        animation.render();           // but only render once please

        // or alternatively to call the code the number of missed periods
        for (auto periods{timer.wait()}; periods; periods--)
        {
            // periods is decreasing!
        }
        // This is fine, since execute() is evaluated only once!
        for (auto periods : modm::range(timer.wait()))
        {
            // periods is increasing!
        }
        // THIS WILL NOT WORK, since execute() reschedules itself immediately!
        for (auto periods{0}; periods < timer.wait(); periods++)
        {
            // called at most only once!!! periods == 0 always!
        }
    }
});
```

!!! warning "DO NOT use for hard real time systems!"
    You are responsible for polling these timers `execute()` methods as often as
    required. If you need to meet hard real time deadlines these are not the
    timers you are looking for!

!!! note "Timers are stopped by default!"
    If you want to start a timer at construction time, give the constructor a
    duration. Duration Zero will expire the timer immediately


## Resolution

Two timer resolutions are available, using `modm::Clock` for milliseconds and
`modm::PreciseClock` for microseconds. They follow the same naming scheme:

- `modm::Timeout`, `modm::PeriodicTimer`: 49 days in milliseconds and 8 bytes.
- `modm::PreciseTimeout`, `modm::PrecisePeriodicTimer`: 71 mins in microseconds
  and 8 bytes.

If you deal with short time periods, you can save a little memory by using the
16-bit versions of the same timers:

- `modm::ShortTimeout`, `modm::ShortPeriodicTimer`: 65 seconds in milliseconds
  and 4 bytes.
- `modm::ShortPreciseTimeout`, `modm::ShortPrecisePeriodicTimer`: 65
  milliseconds in microseconds and 4 bytes.
