# try to make a radioalert clock with bose soundtouch

- autodiscover soundtouch devices (mdns/bonjour)
- simple web gui
- alerts on bose soundtouch devices

- small api for requests (return json)
  - /api/v1/version: software version
  - /api/v1/info : idf/platformio version, count of cpu cores
  - /api/v1/alerts : get all alerts
  - /api/v1/devices : get all devices where found via mDNS
  - /api/v1/alert=alert=alert_XX : get one alert
  - TODO: /api/v1/set-loglevel?level=7 : set controller loglevel, only app



- small api for requests (return json)
  - /api/v1/version: software version
  - /api/v1/info : idf/platformio version, count of cpu cores
  - /api/v1/set-datalog?server=192.168.1.44&port=63420 : set data log destination (UDP), IP, Port / IP 0 disable
  - /api/v1/set-syslog?server=192.168.1.44&port=512 : set syslog destination (UDP), IP, Port  / IP 0 disable
  - /api/v1/set-timezone?timezone=CET-1 : set internal timezone (for GUI)
  - /api/v1/set-timezone?timezone-offset=3600 : set timezone offset from GMT (workarround for timezone bug)
  - /api/v1/set-loglevel?level=7 : set controller loglevel, only app



## liglevels (numeric)
    EMERGENCY = 0,
    ALERT = 1,
    CRITICAL = 2,
    ERROR = 3,
    WARNING = 4,
    NOTICE = 5,
    INFO = 6,
    DEBUG = 7,








  
