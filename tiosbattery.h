#ifndef TIOSBATTERY_H
#define TIOSBATTERY_H


class TIOSBattery
{
    public:
        typedef enum BSTATE
        {
            BS_UNKNOWN,
            BS_UNPLUGGED,
            BS_CHARGING,
            BS_FULL
        }BSTATE;

        TIOSBattery();

        void update();
        int getBatteryLeft() { return mLeft; }
        int getBatteryState() { return mState; }

    private:
        int mLeft{0};               // The left battery %
        BSTATE mState{BS_UNKNOWN};  // The state (plugged, charging, ...)
};

#endif // TIOSBATTERY_H
