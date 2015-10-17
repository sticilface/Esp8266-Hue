// to experiment with __attribute__((packed))


struct HueXYColor {
  float x{0.3127f};
  float y{0.3290f};
};

struct HueHSB {
  	uint16_t H{65280};
  	uint8_t S{1};
  	uint8_t B{254};
};

struct HueLight {
  char Name[32];
  HueHSB hsb; 
  uint16_t Ct{200};
  HueXYColor xy;  
  bool State{false};
  uint16_t Transitiontime{10};
  char Colormode[3]{'h','s','\0'}; // "hs" = hue, "ct" = colour temp, "xy" = xy   
};


struct HueGroup {
  char Name[32];
  uint16_t Hue{65280};
  uint8_t Sat{1};
  uint8_t Bri{254};
  uint16_t Ct{200};
  HueXYColor xy;  
  bool State{false};
  uint8_t LightMembers[MaxLightMembersPerGroup];
  uint8_t LightsCount{0};
  char Colormode[3]{'h','s','\0'}; // "hs" = hue, "ct" = colour temp, "xy" = xy   
  //char Colormode[3]; // "hs" = hue, "ct" = colour temp, "xy" = xy   
  bool Inuse{false}; 

};


