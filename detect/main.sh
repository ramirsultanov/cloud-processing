#!/bin/bash
set -e
jpg=`jq -r ".messages[0].details.object_id" $1`
echo $jpg
input=$(aws --endpoint-url=https://storage.yandexcloud.net s3 cp s3://${BUCKET}/$jpg - | base64)
result=$(curl -H "Authorization: Bearer `yc --cloud-id ${CLOUD_ID} --folder-id ${FOLDER_ID} iam create-token`" \
    "https://vision.api.cloud.yandex.net/vision/v1/batchAnalyze" \
    -d @<(cat << EOF  
{
    "folderId": "${FOLDER_ID}",
    "analyze_specs": [{
        "content": "$input",
        "features": [{
            "type": "FACE_DETECTION"
        }]
    }]
}
EOF
))
echo $result
queueurl=$(aws sqs get-queue-url --queue-name ${QUEUE} --endpoint https://message-queue.api.cloud.yandex.net/ | jq -r ".QueueUrl")
echo $queueurl
jq -c '.results[0].results[0].faceDetection.faces[]' <<< $result | while read i; do
    json=$(jq -n --arg source "$jpg" --argjson obj $i '{source: $source, obj: $obj}')
    echo "$json"
    aws sqs send-message \
        --endpoint https://message-queue.api.cloud.yandex.net/ \
        --message-body "$json" \
        --queue-url $queueurl
done
