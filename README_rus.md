# <a id="Top">Tuya CO2 Sensor Zigbee on MH-Z19E with custom firmware</a>

### Custom firmware for Tuya CO2 sensor models

1. Model r01
	- "_TZE200_ogkdpgy2"
	- "_TZE204_ogkdpgy2"
2. Model r02
	- "_TZE204_yvx5lh6k"

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/sensor.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/board.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/board2.jpg"/>

**Автор не несет никакой отвественности, если вы, воспользовавшись этим проектом, превратите свой умный датчик CO2 в полоумный.**

Если у вас другая сигнатура, лучше не заливать, не проверив на совпадение датапоинтов.

Проверялся только в zigbee2mqtt.

## Зачем. 

Чтобы не спамил в сеть. Шлет данные каждые полторы секунды.

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/spam.jpg"/>

## Что получилось. 

**About**

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/about.jpg"/>

**Exposes**

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/exposes.jpg"/>

**Reporting**

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/reporting.jpg"/>


## Как обновить.

Сначала подключаем к z2m один внешний [конвертор](https://github.com/slacky1965/tuya_co2sensor_zrd/tree/main/zigbee2mqtt/convertors) `tuya_co2sensor_orig.js`. Он активирует OTA в z2m для датчика с прошивкой от Tuya.

Далее нужно добавить локальное хранилище обновлений. 

Создаем директорию `images` в директории z2m и кладем туда файл [1141-d3a3-1111114b-tuya_co2sensor_zrd.zigbee](https://github.com/slacky1965/tuya_co2sensor_zrd/raw/refs/heads/main/bin/1141-d3a3-1111114b-tuya_co2sensor_zrd.zigbee).

Копируем в директорию z2m файл [local_ota_index.json](https://github.com/slacky1965/tuya_co2sensor_zrd/blob/main/zigbee2mqtt/local_ota_index.json)

В конфиг z2m `configuration.yaml` добавляем локальное хранилище

```
ota:
  zigbee_ota_override_index_location: local_ota_index.json
```

А конвертор `tuya_co2sensor_orig.js` кладем в директорию `external_converters`, которую нужно создать в корне z2m.

Далее перегружаем z2m. И видим у нас новое устройство.

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/tuya_ready.jpg"/>

Далее идем в раздел OTA. И видим там свое устройство. Жмем проверить обновления.

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/update.jpg"/>
	
<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/check_update.jpg"/>

Жмем на красную кнопку. И обновляемся.

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/update_1.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/update_2.jpg"/>

Если все не так, как описано, значит вы что-то сделали не по инструкции (не положили файл куда нужно, не перегрузили z2m) или сигнатуры вашего датчика нет в списке поддерживаемых устройств.

> [!WARNING]
> Внимание!!! Если в процессе вы обнаружите на каких-то устройствах Туя, которые возможно у вас есть еще в системе, новое обновление, то обновлять ничего не нужно!!! Иначе вы зальете в это устройство прошивку от датчика и получите кирпич!!! Если же процесс обновления по ошибке уже начался, то просто обесточьте это устройство!!!

Далее ждем окончания. Вот так выглядит лог при первом старте после обновления с прошивки Tuya на кастомную.

```
OTA mode enabled. MCU boot from address: 0x0
Firmware version: v1.0.01
Tuya bootloader
Bootloader is overwritten. Reset
OTA mode enabled. MCU boot from address: 0x0
Firmware version: v1.0.01
SDK bootloader
out_pkt <== 0x55AA02000101000003
inp_pkt ==> 0x55AA02000101001C7B2270223A226F676B6470677932222C2276223A22312E302E30227DC1
Tuya signature found: "ogkdpgy2"
Use modelId: Tuya_CO2Sensor_r01
```

После обновления нужно удалить устройство из z2m. Перегрузить z2m. Разрешить сопряжение. Если на датчике светодиод горит, то больше делать ничего не нужно. Если не горит, то зажать кнопку на 5 секунд и отпустить. Светодиод должен загореться. Начнется сопряжение.

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/joined.jpg"/>

Все, датчик готов к работе.

В Home Assistant это выглядит так

<img src="https://raw.githubusercontent.com/slacky1965/tuya_co2sensor_zrd/refs/heads/main/doc/images/ha.jpg"/>

В середине провал - это я ушел гулять с собаками и открыл все форточки.

Все.

P.S. В реальной работе не проверялось, требует всестороннего тестирования.

---

Связаться со мной можно в **[Telegram](https://t.me/slacky1965)**.

### Если захотите отблагодарить автора, то это можно сделать через [ЮMoney](https://yoomoney.ru/to/4100118300223495)

---

Спасибы :))

- [@upavlel](https://t.me/upavlel) за предоставленный датчик CO2 `_TZE200_ogkdpgy2` на растерзание.
- [@ekurdiukov](https://t.me/ekurdiukov) за предоставленный датчик `_TZE204_yvx5lh6k` для проверки.


## История версий
- 1.0.01
	- Начало.
- 1.0.02
	- Из основной прошивки удален код проверки и перезаписи `bootloader'а` - это была потенциальная угроза порчи boot-сектора при определенных обстаятельствах. Теперь этот код содержится только в прошивке, которая загружается при первом обновлении.
	- При первом обновлении номер версии всегда будет `1.0.00` - сделано специально для скорейшего обновления на основную прошивку, где нет кода проверки и перезаписи `bootloader'а`.
- 1.0.03
	- Добавлен датчик с сигнатурой `_TZE204_yvx5lh6k`.
- 1.0.04
	- Добавлен датчик с сигнатурой `_TZE200_yvx5lh6k`.
	- Устранен баги с репортингом. Не правильно сравнивал тип float. При MaxInterval равным 0 получали "кирпич" (баг SDK).

[Наверх](#Top)

