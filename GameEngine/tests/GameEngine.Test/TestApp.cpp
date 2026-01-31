#include <cstdio>

// Simple bridge: reuse the BlueBox2D headless demo (box + bouncing text).
int RunBlueBox2DTest();

int main()
{
    int rc = RunBlueBox2DTest();
    if (rc == 0)
    {
        std::puts("[GameEngine.Test] Completed BlueBox2DTest (see BlueBox2DTest_output.bmp)");
    }
    else
    {
        std::puts("[GameEngine.Test] BlueBox2DTest failed");
    }
    return rc;
}
