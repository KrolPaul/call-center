### Сборка
##### Колл-центр
```
cd call-center
mkdir build
cd build
cmake ..
cmake --build . --config Release --target CallCenter
```
Для отключения логов определенного уровня необходимо раскомментировать необходимые директивы в CallCenter/СMakeLists.txt (add_compile_definitions). Также возможно отключение вывода логов в стандартный поток вывода и/или файл.
Описание директив для изменения параметров логирования: https://github.com/abumq/easyloggingpp/blob/master/README.md#configuration-macros
##### Тесты
```
cd call-center
mkdir build
cd build
cmake ..
cmake --build . --config Release --target tests
```
### Запуск
##### Запуск колл-центра
Параметры командной строки:
1. Хост сервера
2. Порт сервера
3. Время между перечитыванием конфигурации (секунды) (опционально). Если не указано, то конфигурация не будет перечитываться.

```
./CallCenter 127.0.0.1 7777 600
```
##### Запуск тестов
```
./tests/tests
```

### Описание работы программы
Программа эмулирует работу колл-центра. Звонки приходят HTTP запросами с указанием номера телефона. Формат запроса: **http:/host:port/call?phone_number=**.
Все входящие звонки попадают в очередь, в которой ожидают ответа оператора. HTTP ответ отправляется по факту постановки в очередь (статус звонка на данном этапе определяется статусом постановки в очередь, а не конечным статусом звонка). Из очереди звонок может выйти по таймауту или же быть обслужен оператором.

В программе реализовано перечитывание файла конфигурации по таймеру. При изменении кол-ва операторов или мест в очереди в меньшую сторону текущие обслуживаемые звонки и звонки, находящиеся в очереди, не удаляются. Переход к меньшему кол-ву производится плавно по мере освобождения операторов и мест в очереди.
#### Создание звонков
Для создания звонка необходимо отправить HTTP GET по **http:/host:port/call?phone_number=** с заданным номером телефона, где host, port - хост и порт, указанные при запуске программы.
Программа отправит ответ в формате:
```
{"call_id" : 16399222369993846635,
"call_status" : "ok"
}
```
|   call_status                |                                                      Описание          |
|------------------------|------------------------------------------------------------------------|
| ok                              | Звонок поставлен в очередь.                |
| overload                    | Звонок не поставлен в очередь. Очередь переполнена.    |
| alreadyInQueue        | Звонок не поставлен в очередь. Звонок с заданным номером уже находится в очереди. (Возможно только при rejectRepeatedCalls = true)|

### Конфигурирование
Конфигурация колл-центра описыватся в файле **call-center.json** в формате Json. Конфигурация *по умолчанию* находится в файле **default-call-center.json**. При ошибке получения конфигурации *по умолчанию* (отсутствие файла или ошибки в параметрах) производится аварийный останов программы.
##### Пример файла конфигурации
```
{ "minResponseTime" : 0,
  "maxResponseTime" : 150,
  "minCallDuration" : 10,
  "maxCallDuration" : 300,
  "nOperators" : 10,
  "rejectRepeatedCalls" : false,
  "maxCallQueueSize" : 10
  }
```
##### Параметры

|   Параметр  |                                                                                           Описание          |
|------------------------|------------------------------------------------------------------------|
| minResponseTime   | Минимальное время нахождения звонка в очереди.                |
| maxResponseTime   | Максимальное время нахождения звонка в очереди.             |
| minCallDuration    | Минимальная продолжительность звонка. (>0) |
| maxCallDuration    | Максимальная продолжительность звонка. (>0) |
| nOperators    | Количество операторов.    (>0)       |
| rejectRepeatedCalls | true - отклонять звонки от номеров телефона, уже состоящих в очереди. false - если звонок с данным номером телефона уже находится в очереди, то он удаляется из очереди, а новый звонок ставится в конец очереди.      |
|maxCallQueueSize | Количество мест в очереди звонков.  (>0) |