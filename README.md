TinyProfiler for C / C++
------------------------

License: Public Domain

Supported architectures: Linux (x64), Windows (x64, UWP)

<img width="800px" src="https://i.imgur.com/oQZgs0H.jpg" />

Usage:

```C
#define USE_TINYPROFILER
#include "tinyprofiler.h"

int main() {
  profAlloc(1000000);
  profB("main");
  ...
  profE("main");
  profPrintAndFree();
}
```

Redirect console output to a `.json` file and open it in `about:tracing` tab of your web browser.