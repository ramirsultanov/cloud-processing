# cloud-processing

## Author | Автор

#### Ramir Sultanov 904 | Рамир Султанов 904

## Dependencies | Зависимости

1. terraform

## Setup | Настройка

#### In order to setup system you should apply terraform configuration to your cloud:
#### Чтобы настроить систему, вы должны применить конфигурацию terraform к своему облаку:
```console
user@workstation:<repository-root-directory>$ terraform apply
```
#### The configuration tool will ask you for 'var.cloud_id' that is your Cloud ID, 'var.folder_id' that is your Folder ID, and 'var.telegram_bot_api_key' that is your telegram bot API key which you should have been receieved after creating your bot. Please fill this fields and configuration should e applied automatically. If it is not then you should resolve issues manually. For example, if there was no enough free space for Yandex Managed Service for YDB then you can create this service manually, delete its creation code from the 'main.tf' file and use Document API Endpoint for this service instead of 'ENDPOINT_URL_DB' in the 'main.tf' file. After configuration is done you should manually create trigger 'vvot08-task-trigger' for Message Queue because of limitations of current terraform yandex provider version:
#### Инструмент настройки запросит у вас 'var.cloud_id', который является вашим идентификатором облака, 'var.folder_id', который является идентификатором вашей папки, и 'var.telegram_bot_api_key', который является вашим ключом API от telegram бота, который вы должны были получить после создания вашего бота. Пожалуйста, заполните эти поля, и конфигурация должна быть применена автоматически. Если это не так, то вам следует устранить неполадки вручную. Например, если не хватило свободного места для управляемого сервиса Яндекса для YDB, то вы можете создать этот сервис вручную, удалив его код создания из файла 'main.tf ' и использовать конечную точку Document API для этой службы вместо 'ENDPOINT_URL_DB' в файле 'main.tf'. После завершения настройки вам следует вручную создать триггер 'vvot08-task-trigger' для очереди сообщений из-за ограничений текущей версии провайдера yandex в terraform:
```console
user@workstation:<repository-root-directory>$ yc serverless trigger create message-queue \
  --name <trigger_name> \
  --queue <queue_ID> \
  --queue-service-account-id <service_account_ID> \
  --invoke-container-id <container_ID> \
  --invoke-container-service-account-id <service_account_ID> \
  --batch-size 1 \
  --batch-cutoff 10s
```
#### where
#### где
1. <trigger_name\> is the name for your trigger, for example: 'vvot08-task-trigger' | это имя вашего триггера, например: 'vvot08-task-trigger'.
2. <queue_ID\> is the ID of the queue which name by default is 'vvot08-tasks' | это идентификатор очереди, имя которой по умолчанию 'vvot08-tasks'.
3. <service_account_ID\> is the ID of the service account with a name by default 'my-robot' | это идентификатор учетной записи службы с именем по умолчанию 'my-robot'.
4. <container_ID\> is the ID of the container with a name by default 'vvot08-face-cut' | это идентификатор контейнера с именем по умолчанию 'vvot08-face-cut'.
#### After that is done you could set webhook for your telegram bot:
#### После этого вы можете установить webhook для своего telegram-бота:
```console
user@workstation:<repository-root-directory>$ curl   --request POST   --url https://api.telegram.org/bot<BOT_API_KEY>/setWebhook   --header 'content-type: application/json'   --data '{"url": "<CLOUD_FUNCTION_URL>"}'
```
#### where
#### где
1. <BOT_API_KEY\> is the API key of your telegram bot | это API ключ вшего telegram-бота.
2. <CLOUD_FUNCTION_URL\> is the URL of your cloud function with a name by default 'vvot08-boot' | это URL вашей облачной функции с именем по умолчанию 'vvot08-boot'
#### After that is done your system is ready to go!
#### После того, как это будет сделано, ваша система готова к работе!

## Usage | Использование

#### After image was uploaded to the bucket with a name by default 'itis-2022-2023-vvot08-photos' it will be processed automatically by the system.
#### После того, как изображение было загружено в корзину с именем по умолчанию 'itis-2022-2023-vvot08-photos" оно будет автоматически обработано системой.


### Getting face image without name.
### Получение изображения лица без имени.

1. Send the '/getface' command to your telegram bot. | Отправьте команду "/getface" своему telegram-боту.
2. Receive image of a face without a name or 'Фотографии без имени не найдены' in case there was no images of face without name. | Получите изображение лица без имени или "Фотографии без имени не найдены" в случае, если не было найдено изображений лица без имени.

### Naming face image without a name.
### Присвоение имени изображению лица без имени.

1. Send the '/getface' command to your telegram bot. | Отправьте команду '/getface' своему telegram-боту.
2. Receive image of a face without a name or 'Фотографии без имени не найдены' in case there was no images of a face without name. | Получите изображение лица без имени или "Фотографии без имени не найдены" в случае, если не было найдено изображений лица без имени.
3. If you have received an image of a face you can name it. To do that you should just send a name that should be applied to received image. | Если вы получили изображение лица, вы можете назвать его. Для этого вам следует просто отправить имя, которое должно быть применено к полученному изображению.

### Getting face images with a name.
### Получение изображений лиц с именем.

1. Send the '/find <name\>' command to your telegram bot where <name\> is the name you want to find. | Отправьте команду '/find <name\>' своему telegram-боту, где <name\> - это имя, которое вы хотите найти.
2. Receive images of faces with that name or 'Фотографии с <name\> не найдены' in case there was no images of faces with name <name\>. | Получите изображения лиц с этим именем или 'Фотографии с <name\> не найдены' в случае, если не было найдено изображений лиц с именем <name\>.
