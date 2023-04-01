# esp8266_json_menu

Simple remote JSON menu browser with OLED display module and rotary encoder.

Demo:

https://user-images.githubusercontent.com/412987/229306925-b53eb4c4-7d26-4912-a236-6db12c4df904.mp4


Sample menu json:
```
{
  "title": "MajorDoMo",
  "refresh": 600,
  "items": [
    {
      "title": "Switch 1",
      "request": "/objects/?method=Dimmer12.switch"
    },
    {
      "title": "Switch 2",
      "url": "/menu2.json"
    }
  ]
}
```

__title__ - menu title  
__refresh__ (optional) - refresh rate (seconds) -- after given time menu will be reloaded automatically  
__items__ - list of menu items  
__items.title__ - menu item ititle  
__items.request__ (optional) - URL for request to be sent when user hits menu element  
__items.url__ (optional) - URL for menu to load when user hits menu element

### Hardware

__Controller:__ Wemos D1 mini

__Display:__

Oled i2c Display 128×64  
SCREEN_SDA connected to D2  
SCREEN_SCL connected to D1

__Rotary encoder:__

ROTARY_ENCODER_A_PIN connected to D5  
ROTARY_ENCODER_B_PIN connected to D6  
ROTARY_ENCODER_BUTTON_PIN connected to D7
