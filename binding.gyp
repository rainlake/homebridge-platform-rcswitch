{
      "targets": [
        {
          "target_name": "rsswitch",
          "sources": [
            "rcswitch.c", "rcmodule.cpp"
          ],
          "include_dirs": [
            ""
          ],
          "libraries": [
            "-luv", "-lwiringPi"
          ],
          "cflags!": [ "-fno-exceptions" ],
          "cflags": [ "-std=c++11" ],
          "cflags_cc!": [ "-fno-exceptions" ]
        }
      ]
    }
