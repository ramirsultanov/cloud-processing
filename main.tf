terraform {
  required_providers {
    yandex = {
      source = "yandex-cloud/yandex"
    }
  }
  required_version = ">= 0.13"
}

provider "yandex" {
  zone = "ru-central1-a"
}

resource "yandex_iam_service_account_static_access_key" "sa-static-key" {
  service_account_id = var.said
  description        = "static access key for object storage"
}

resource "yandex_storage_bucket" "photos" {
  access_key = yandex_iam_service_account_static_access_key.sa-static-key.access_key
  secret_key = yandex_iam_service_account_static_access_key.sa-static-key.secret_key
  bucket     = var.photosb
}

resource "yandex_storage_bucket" "faces" {
  access_key = yandex_iam_service_account_static_access_key.sa-static-key.access_key
  secret_key = yandex_iam_service_account_static_access_key.sa-static-key.secret_key
  bucket     = var.facesb
}

resource "yandex_message_queue" "vvot-tasks" {
  name                        = "vvot${var.id}-tasks"
  visibility_timeout_seconds  = 600
  receive_wait_time_seconds   = 20
  message_retention_seconds   = 1209600
  access_key                  = yandex_iam_service_account_static_access_key.sa-static-key.access_key
  secret_key                  = yandex_iam_service_account_static_access_key.sa-static-key.secret_key
}

resource "yandex_function" "vvot-face-detection" {
    name               = "vvot${var.id}-face-detection"
    description        = "vvot${var.id}-face-detection"
    user_hash          = "hash"
    runtime            = "bash"
    entrypoint         = "main.sh"
    memory             = "128"
    execution_timeout  = "10"
    service_account_id = var.said
    tags               = ["no_tag"]
    environment        = {
        AWS_ACCESS_KEY_ID = yandex_iam_service_account_static_access_key.sa-static-key.access_key
        AWS_REGION = "ru-central1"
        AWS_SECRET_ACCESS_KEY = yandex_iam_service_account_static_access_key.sa-static-key.secret_key
        BUCKET = var.photosb
        CLOUD_ID = var.cloud_id
        ENDPOINT_URL = "https://message-queue.api.cloud.yandex.net"
        FOLDER_ID = var.folder_id
        QUEUE = "vvot${var.id}-tasks"
    }
    content {
        zip_filename = "detect.zip"
    }
}

output "yandex_function_face_detection" {
    value = "${yandex_function.vvot-face-detection.id}"
}

resource "yandex_function_trigger" "vvot-photo-trigger" {
    name        = "vvot${var.id}-photo-trigger"
    description = "vvot${var.id}-photo-trigger"
    object_storage {
        bucket_id = var.photosb
        create    = true
        suffix    = ".jpg"
    }
    function {
        id                 = "${yandex_function.vvot-face-detection.id}"
        service_account_id = var.said
    }
}

resource "yandex_ydb_database_serverless" "vvot-db-photo-face" {
  name      = "vvot${var.id}-db-photo-face"
}

resource "yandex_serverless_container" "vvot-face-cut" {
    name               = "vvot${var.id}-face-cut"
    description        = "vvot${var.id}-face-cut"
    memory             = 128
    service_account_id = var.said
    image {
        url = "cr.yandex/crp5kgdmgi049mag05a7/vvot08-face-cut:221219070000"
        environment = {
            AWS_ACCESS_KEY_ID = yandex_iam_service_account_static_access_key.sa-static-key.access_key
            AWS_REGION = "ru-central1"
            AWS_SECRET_ACCESS_KEY = yandex_iam_service_account_static_access_key.sa-static-key.secret_key
            BUCKET_SRC = var.photosb
            BUCKET_DST = var.facesb
            CLOUD_ID = var.cloud_id
            ENDPOINT_URL_BUCKET = "https://storage.yandexcloud.net"
            ENDPOINT_URL_DB = "${yandex_ydb_database_serverless.vvot-db-photo-face.document_api_endpoint}"
            ENDPOINT_URL_QUEUE = "https://message-queue.api.cloud.yandex.net"
            FOLDER_ID = var.folder_id
            QUEUE = "${yandex_message_queue.vvot-tasks.id}"
        }
    }
}

resource "yandex_api_gateway" "vvot-api-gateway" {
  name        = "itis-2022-2023-vvot${var.id}-api"
  description = "itis-2022-2023-vvot${var.id}-api"
  labels      = {
  }
  spec = <<-EOT
    openapi: "3.0.0"
    info:
      version: 1.0.0
      title: VVOT API
    paths:
      /:
        get:
          summary: get object
          operationId: default
          parameters:
            - name: face
              in: query
              description: name
              required: true
              schema:
                type: string
          x-yc-apigateway-integration:
            type: object_storage
            bucket: "${var.facesb}"
            object: '{face}'
            error_object: error.html
            presigned_redirect: true
            service_account_id: "${var.said}"
  EOT
}

resource "yandex_function" "vvot-boot" {
    name               = "vvot${var.id}-boot"
    description        = "vvot${var.id}-boot"
    user_hash          = "hash"
    runtime            = "golang119"
    entrypoint         = "index.Handle"
    memory             = "128"
    execution_timeout  = "10"
    service_account_id = var.said
    tags               = ["no_tag"]
    environment        = {
        AWS_ACCESS_KEY_ID = yandex_iam_service_account_static_access_key.sa-static-key.access_key
        AWS_REGION = "ru-central1"
        AWS_SECRET_ACCESS_KEY = yandex_iam_service_account_static_access_key.sa-static-key.secret_key
        BUCKET_SRC = var.photosb
        BUCKET_DST = var.facesb
        CLOUD_ID = var.cloud_id
        ENDPOINT_URL_BUCKET = "https://storage.yandexcloud.net"
        ENDPOINT_URL_DB = "${yandex_ydb_database_serverless.vvot-db-photo-face.document_api_endpoint}"
        ENDPOINT_URL_QUEUE = "https://message-queue.api.cloud.yandex.net"
        FOLDER_ID = var.folder_id
        QUEUE = "${yandex_message_queue.vvot-tasks.id}"
        TLG_API = var.telegram_bot_api_key
        GATEWAY = "${yandex_api_gateway.vvot-api-gateway.domain}"
    }
    content {
        zip_filename = "boot.zip"
    }
}

resource "yandex_function_iam_binding" "vvot-boot-iam" {
  function_id = "${yandex_function.vvot-boot.id}"
  role        = "serverless.functions.invoker"

  members = [
    "system:allUsers",
  ]
}
