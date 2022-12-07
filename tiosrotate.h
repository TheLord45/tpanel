#ifndef TIOSROTATE_H
#define TIOSROTATE_H


class TIOSRotate
{
    public:
        TIOSRotate();

        static void rotate(int dir);
        static void setAllowedOrientations(bool portrait) { mPortrait = portrait; }
        static bool getAllowedOrientation() { return mPortrait; }
        static void automaticRotation(bool allow);

    private:
        static bool mPortrait;
};

#endif // TIOSROTATE_H
