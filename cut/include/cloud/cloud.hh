#ifndef CLOUD_CLOUD_HH_
#define CLOUD_CLOUD_HH_

#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/sqs/SQSClient.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <aws/sqs/model/SendMessageResult.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/sqs/model/ReceiveMessageResult.h>
#include <aws/sqs/model/DeleteMessageRequest.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/WebsiteConfiguration.h>
#include <aws/s3/model/PutBucketPolicyRequest.h>
#include <aws/s3/model/PutBucketWebsiteRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/CreateBucketConfiguration.h>
#include <aws/s3/model/PutPublicAccessBlockRequest.h>
#include <aws/s3/model/PutBucketAclRequest.h>
#include <aws/core/utils/Outcome.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/AttributeDefinition.h>
#include <aws/dynamodb/model/CreateTableRequest.h>
#include <aws/dynamodb/model/KeySchemaElement.h>
#include <aws/dynamodb/model/ProvisionedThroughput.h>
#include <aws/dynamodb/model/ScalarAttributeType.h>
#include <aws/dynamodb/model/ListTablesRequest.h>
#include <aws/dynamodb/model/ListTablesResult.h>
#include <aws/dynamodb/model/AttributeDefinition.h>
#include <aws/dynamodb/model/PutItemRequest.h>
#include <aws/dynamodb/model/PutItemResult.h>
#include <iostream>

#include <fstream>
#include <filesystem>
#include <vector>
#include <set>
#include <string>
#include <random>
#include <cstdlib>
#include <memory>
#include <cstddef>
#include <optional>

#ifdef __linux__
#include <unistd.h>
#endif

#include <opencv2/opencv.hpp>

#include <jsoncpp/json/json.h>

namespace cloud {

class Cloud {
public:
  Cloud();
  bool init();
  bool deinit();
//      const std::string& region = "ru-central1",
//      const std::string& endpoint = "https://message-queue.api.cloud.yandex.net"
  bool send(const std::string& expr);
  std::string receive();
  bool put(const std::string& data, std::string key) const;
  std::optional<std::string> put(const std::string& data) const;
  bool objectExists(const std::string& key) const;
  bool tableExists(const std::string& name) const;
  bool tableCreate(const std::string& name) const;
  bool tableSave(
      const std::string& table,
      const std::string& object,
      const std::string& from
  ) const;
  cv::Mat image(const std::string& key) const;
  static std::tuple<std::string, std::array<cv::Point, 4>> parse(const std::string& str);
protected:
  std::optional<Aws::SQS::SQSClient> sqs_;
  std::optional<Aws::S3::S3Client> s3_;
  std::unique_ptr<Aws::DynamoDB::DynamoDBClient> dynamo_;
  Aws::SDKOptions options_;
  std::string bucketSrc_;
  std::string bucketDst_;
  std::string queue_;

  static constexpr auto BUCKET_SRC_KEY = "BUCKET_SRC";
  static constexpr auto BUCKET_DST_KEY = "BUCKET_DST";
  static constexpr auto KEY_ID_KEY = "AWS_ACCESS_KEY_ID";
  static constexpr auto SECRET_KEY_KEY = "AWS_SECRET_ACCESS_KEY";
  static constexpr auto REGION_KEY = "AWS_REGION";
  static constexpr auto ENDPOINT_KEY_BUCKET = "ENDPOINT_URL_BUCKET";
  static constexpr auto ENDPOINT_KEY_DB = "ENDPOINT_URL_DB";
  static constexpr auto ENDPOINT_KEY_QUEUE = "ENDPOINT_URL_QUEUE";
  static constexpr auto QUEUE_KEY = "QUEUE";
private:
};

} /// namespace cloud

namespace util {

std::string& replace(
    std::string& object,
    const std::string& key,
    const std::string& replacement
);

std::string urlEncode(const std::string& value);

std::string read(const std::filesystem::path& path);

} /// namespace util

/// implementation

namespace cloud {

Cloud::Cloud() {
  this->queue_ = std::getenv(QUEUE_KEY);
}

bool Cloud::init() {
  options_.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug;
  Aws::InitAPI(options_);
  Aws::Auth::AWSCredentials credentials;
  {
    const auto keyId = std::getenv(KEY_ID_KEY);
    if (std::string(keyId).empty()) {
      return false;
    }
    credentials.SetAWSAccessKeyId(Aws::String(keyId));
    const auto key = std::getenv(SECRET_KEY_KEY);
    if (std::string(key).empty()) {
      return false;
    }
    credentials.SetAWSSecretKey(Aws::String(key));
  }
  {
    Aws::Client::ClientConfiguration config;
    {
      config.region = Aws::String(std::getenv(REGION_KEY));
      if (config.region.empty()) {
        return false;
      }
      config.endpointOverride = Aws::String(std::getenv(
         ENDPOINT_KEY_QUEUE)
      );
      if (config.endpointOverride.empty()) {
        return false;
      }
    }
    sqs_ = Aws::SQS::SQSClient(credentials, config);
  }
  {
    Aws::Client::ClientConfiguration config;
    {
      config.region = Aws::String(std::getenv(REGION_KEY));
      if (config.region.empty()) {
        return false;
      }
      config.endpointOverride = Aws::String(std::getenv(
         ENDPOINT_KEY_BUCKET)
      );
      if (config.endpointOverride.empty()) {
        return false;
      }
    }
    s3_ = Aws::S3::S3Client(credentials, config);
  }
  {
    Aws::Client::ClientConfiguration config;
    {
      config.region = Aws::String(std::getenv(REGION_KEY));
      if (config.region.empty()) {
        return false;
      }
      config.endpointOverride = Aws::String(std::getenv(
         ENDPOINT_KEY_DB)
      );
      if (config.endpointOverride.empty()) {
        return false;
      }
    }
    dynamo_ = std::make_unique<Aws::DynamoDB::DynamoDBClient>(credentials, config);
  }
  bucketSrc_ = std::getenv(BUCKET_SRC_KEY);
  if (bucketSrc_.empty()) {
    return false;
  }
  bucketDst_ = std::getenv(BUCKET_DST_KEY);
  if (bucketDst_.empty()) {
    return false;
  }

  return true;
}

bool Cloud::deinit() {
  Aws::ShutdownAPI(options_);
  return true;
}

bool Cloud::send(const std::string& expr) {
  Aws::SQS::Model::SendMessageRequest sm_req;
  sm_req.SetQueueUrl(this->queue_);
  sm_req.SetMessageBody(expr);
  std::random_device rd;
  std::mt19937 gen{rd()};
  std::uniform_int_distribution<short> d;
  sm_req.SetMessageGroupId(std::to_string(d(gen)));
  sm_req.SetMessageDeduplicationId(std::to_string(d(gen)));

  auto sm_out = sqs_->SendMessage(sm_req);
  if (sm_out.IsSuccess())
  {
    return true;
  }
  else
  {
    std::cerr << "Error sending message to " << this->queue_ << ": " <<
        sm_out.GetError().GetMessage() << std::endl;
    return false;
  }
}

std::string Cloud::receive() {
  Aws::SQS::Model::ReceiveMessageRequest rm_req;
  rm_req.SetQueueUrl(this->queue_);
  rm_req.SetMaxNumberOfMessages(1);

  auto rm_out = sqs_->ReceiveMessage(rm_req);
  if (!rm_out.IsSuccess())
  {
    std::cout << "Error receiving message from queue " << this->queue_ << ": "
        << rm_out.GetError().GetMessage() << std::endl;
    return {};
  }

  const auto& messages = rm_out.GetResult().GetMessages();
  if (messages.size() == 0)
  {
    std::cout << "No messages received from queue " << this->queue_ <<
        std::endl;
    return {};
  }

  const auto& message = messages[0];
  std::string ret = message.GetBody();

  Aws::SQS::Model::DeleteMessageRequest dm_req;
  dm_req.SetQueueUrl(this->queue_);
  dm_req.SetReceiptHandle(message.GetReceiptHandle());

  auto dm_out = sqs_->DeleteMessage(dm_req);
  if (dm_out.IsSuccess())
  {
    std::cout << "Successfully deleted message " << message.GetMessageId()
        << " from queue " << this->queue_ << std::endl;
  }
  else
  {
    std::cout << "Error deleting message " << message.GetMessageId() <<
        " from queue " << this->queue_ << ": " <<
        dm_out.GetError().GetMessage() << std::endl;
    return {};
  }

  return ret;
}

bool Cloud::put(const std::string& data, std::string key) const {
  Aws::S3::Model::PutObjectRequest request;
  request.SetBucket(bucketDst_);
  request.SetKey(key);
  const std::shared_ptr<Aws::IOStream> inputData =
        Aws::MakeShared<Aws::StringStream>("");
    *inputData << data;
  request.SetBody(inputData);
  Aws::S3::Model::PutObjectOutcome outcome =
      s3_.value().PutObject(request);
  if (!outcome.IsSuccess()) {
    return false;
  }

  return true;
}

std::optional<std::string> Cloud::put(const std::string& data) const {
  std::size_t key = 0;
  Aws::S3::Model::GetObjectOutcome get_object_outcome;
  do {
    Aws::S3::Model::GetObjectRequest object_request;
    object_request.SetBucket(bucketDst_);
    object_request.SetKey(std::to_string(++key) + ".jpg");

    get_object_outcome =
        s3_.value().GetObject(object_request);
  } while (get_object_outcome.IsSuccess());
  if (!this->put(data, std::to_string(key) + ".jpg")) {
    return {};
  }
  return std::to_string(key) + ".jpg";
}

bool Cloud::objectExists(const std::string& key) const {
  Aws::S3::Model::GetObjectRequest object_request;
  object_request.SetBucket(bucketSrc_);
  object_request.SetKey(key);

  Aws::S3::Model::GetObjectOutcome get_object_outcome =
      s3_.value().GetObject(object_request);

  if (!get_object_outcome.IsSuccess()) {
    return false;
  } else {
    return true;
  }
}

bool Cloud::tableExists(const std::string& name) const {
  Aws::DynamoDB::Model::ListTablesRequest listTablesRequest;
  listTablesRequest.SetLimit(50);
  do
  {
    const Aws::DynamoDB::Model::ListTablesOutcome& lto = this->dynamo_->ListTables(listTablesRequest);
    if (!lto.IsSuccess())
    {
      throw std::runtime_error("couldn't get tables list\nError: " + lto.GetError().GetMessage());
    }

    for (const auto& s : lto.GetResult().GetTableNames()) {
      if (s == name) {
        return true;
      }
    }
    listTablesRequest.SetExclusiveStartTableName(lto.GetResult().GetLastEvaluatedTableName());
  } while (!listTablesRequest.GetExclusiveStartTableName().empty());
  return false;
}

bool Cloud::tableCreate(const std::string& name) const {
  Aws::DynamoDB::Model::CreateTableRequest req;

  Aws::DynamoDB::Model::AttributeDefinition object;
  object.SetAttributeName("object");
  object.SetAttributeType(Aws::DynamoDB::Model::ScalarAttributeType::S);
  req.AddAttributeDefinitions(object);

  Aws::DynamoDB::Model::AttributeDefinition sourceKey;
  sourceKey.SetAttributeName("source");
  sourceKey.SetAttributeType(Aws::DynamoDB::Model::ScalarAttributeType::S);
  req.AddAttributeDefinitions(sourceKey);

  Aws::DynamoDB::Model::AttributeDefinition nameColumn;
  nameColumn.SetAttributeName("name");
  nameColumn.SetAttributeType(Aws::DynamoDB::Model::ScalarAttributeType::S);
  req.AddAttributeDefinitions(nameColumn);

  Aws::DynamoDB::Model::KeySchemaElement keyobject;
  keyobject.WithAttributeName("object").WithKeyType(Aws::DynamoDB::Model::KeyType::HASH);
  req.AddKeySchema(keyobject);

//  Aws::DynamoDB::Model::KeySchemaElement keyname;
//  keyname.WithAttributeName("name").WithKeyType(Aws::DynamoDB::Model::KeyType::RANGE);
//  req.AddKeySchema(keyname);

  Aws::DynamoDB::Model::ProvisionedThroughput thruput;
  thruput.WithReadCapacityUnits(5).WithWriteCapacityUnits(5);
  req.SetProvisionedThroughput(thruput);
  req.SetTableName(name);

  const Aws::DynamoDB::Model::CreateTableOutcome& result = this->dynamo_->CreateTable(req);
  if (result.IsSuccess())
  {
    return true;
  }
  else
  {
    std::cout << "Failed to create table: " << result.GetError().GetMessage();
    return false;
  }
}

bool Cloud::tableSave(
    const std::string& table,
    const std::string& object,
    const std::string& from
) const {
  Aws::DynamoDB::Model::PutItemRequest putItemRequest;
  putItemRequest.SetTableName(table);

  Aws::DynamoDB::Model::AttributeValue av;
  av.SetS(object);

  Aws::DynamoDB::Model::AttributeValue source;
  source.SetS(from);

  putItemRequest.AddItem("object", av);
  putItemRequest.AddItem("source", source);

  const Aws::DynamoDB::Model::PutItemOutcome result =
      this->dynamo_->PutItem(putItemRequest);
  if (!result.IsSuccess())
  {
    std::cout << result.GetError().GetMessage() << std::endl;
    return false;
  }
  return true;
}

cv::Mat Cloud::image(const std::string& key) const {
  Aws::S3::Model::GetObjectRequest object_request;
  object_request.SetBucket(bucketSrc_);
  object_request.SetKey(key);

  Aws::S3::Model::GetObjectOutcome get_object_outcome =
      s3_.value().GetObject(object_request);

  if (!get_object_outcome.IsSuccess()) {
    throw std::runtime_error("couldn't get object with key " + key + "\n" + get_object_outcome.GetError().GetMessage());
  }

  auto& retrieved_file = get_object_outcome.GetResultWithOwnership().
      GetBody();

  std::string data(std::istreambuf_iterator<char>(retrieved_file), {});
  std::vector<unsigned char> vector(data.begin(), data.end());
  cv::Mat res = cv::imdecode(vector, cv::IMREAD_ANYCOLOR);
  return res;
}

std::tuple<std::string, std::array<cv::Point, 4>> Cloud::parse(const std::string& str) {
  constexpr static std::string_view keySource = "source";
  constexpr static std::string_view keyObj = "obj";
  Json::Value j;
  Json::Reader r;
  const auto success = r.parse(str, j);
  if (!success) {
    throw std::runtime_error("couldn't parse json " + str);
  }
  const auto obj = j[std::string(keyObj)];
  const auto boundingBox = obj["boundingBox"];
  const auto vertices = boundingBox["vertices"];
  std::array<cv::Point, 4> parsed;
  for (auto i = 0u; i < parsed.size(); i++) {
    parsed[i].x = std::stoi(vertices[i]["x"].asString());
    parsed[i].y = std::stoi(vertices[i]["y"].asString());
  }
  return {
    j[std::string(keySource)].asString(),
    parsed
  };
}

} /// namespace cloud

namespace util {

std::string& replace(
    std::string& object,
    const std::string& key,
    const std::string& replacement
) {
  auto pos = object.find(key);
  while (pos != std::string::npos) {
    object.replace(pos, key.length(), replacement);
    pos = object.find(key, pos);
  }
  return object;
}

std::string urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (auto i = value.begin(); i != value.end(); i++) {
        std::string::value_type c = (*i);

        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

std::string read(const std::filesystem::path& path) {
  std::ifstream stream(path);
  std::stringstream ss;
  ss << stream.rdbuf();
  std::string contents = ss.str();
  stream.close();
  return contents;
}

} /// namespace util

#endif /// CLOUD_CLOUD_HH_
