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
