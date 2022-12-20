package main

import (
	"log"
	"net/http"
    "os"
    "strings"
    "context"
    "fmt"
    "errors"
	"encoding/json"
	"io/ioutil"

	"github.com/go-telegram-bot-api/telegram-bot-api/v5"
//     "github.com/yandex-cloud/go-sdk"
    "github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
    "github.com/aws/aws-sdk-go-v2/service/dynamodb"
    "github.com/aws/aws-sdk-go-v2/service/dynamodb/types"
    "github.com/aws/aws-sdk-go-v2/feature/dynamodb/expression"
    "github.com/aws/aws-sdk-go-v2/feature/dynamodb/attributevalue"
)

type Response struct {
  StatusCode int         `json:"statusCode"`
  Body       interface{} `json:"body"`
}

type TableBasics struct {
	DynamoDbClient *dynamodb.Client
	TableName      string
}

type Row struct {
	Object string `dynamodbav:"object"`
    Name   string `dynamodbav:"name"`
}

func ReadBody(req *http.Request) ([]byte, error) {
	b, err := ioutil.ReadAll(req.Body)
	defer req.Body.Close()
	return b, err
}

func parseTelegramRequest(req *http.Request) (*tgbotapi.Update, error) {
	b, err := ReadBody(req)
	if err != nil {
		return nil, err
	}
	var update tgbotapi.Update
	err = json.Unmarshal(b, &update)
	if err != nil {
		return nil, err
	}
	return &update, nil
}

func (basics TableBasics) TableExists() (bool, error) {
	exists := true
	_, err := basics.DynamoDbClient.DescribeTable(
		context.TODO(), &dynamodb.DescribeTableInput{TableName: aws.String(basics.TableName)},
	)
	if err != nil {
		var notFoundEx *types.ResourceNotFoundException
		if errors.As(err, &notFoundEx) {
			log.Printf("Table %v does not exist.\n", basics.TableName)
			err = nil
		} else {
			log.Printf("Couldn't determine existence of table %v. Here's why: %v\n", basics.TableName, err)
		}
		exists = false
	}
	return exists, err
}

func (basics TableBasics) Query(name string) ([]Row, error) {
	var err error
	var response *dynamodb.ScanOutput
	var objects []Row
    ex := expression.Name("name").Equal(expression.Value(name))
    projEx := expression.NamesList(
		expression.Name("name"),
		expression.Name("object"))
	expr, err := expression.NewBuilder().WithFilter(ex).WithProjection(projEx).Build()
	if err != nil {
		log.Printf("Couldn't build epxression for query. Here's why: %v\n", err)
	} else {
		response, err = basics.DynamoDbClient.Scan(context.TODO(), &dynamodb.ScanInput{
			TableName:                 aws.String(basics.TableName),
			ExpressionAttributeNames:  expr.Names(),
			ExpressionAttributeValues: expr.Values(),
            FilterExpression:          expr.Filter(),
			ProjectionExpression:      expr.Projection(),
		})
		if err != nil {
			log.Printf("Couldn't query for names in %v. Here's why: %v\n", name, err)
		} else {
			err = attributevalue.UnmarshalListOfMaps(response.Items, &objects)
			if err != nil {
				log.Printf("Couldn't unmarshal query response. Here's why: %v\n", err)
			}
		}
	}
	return objects, err
}

func (basics TableBasics) QueryNullName() ([]Row, error) {
	var err error
	var response *dynamodb.ScanOutput
	var objects []Row
    ex := expression.Name("name").AttributeNotExists()
    projEx := expression.NamesList(
		expression.Name("name"),
		expression.Name("object"))
	expr, err := expression.NewBuilder().WithFilter(ex).WithProjection(projEx).Build()
	if err != nil {
		log.Printf("Couldn't build epxression for query. Here's why: %v\n", err)
	} else {
		response, err = basics.DynamoDbClient.Scan(context.TODO(), &dynamodb.ScanInput{
			TableName:                 aws.String(basics.TableName),
			ExpressionAttributeNames:  expr.Names(),
			ExpressionAttributeValues: expr.Values(),
            FilterExpression:          expr.Filter(),
			ProjectionExpression:      expr.Projection(),
		})
		if err != nil {
			log.Printf("Couldn't scan for names. Here's why: %v\n", err)
		} else {
			// log.Println("response", response)
			err = attributevalue.UnmarshalListOfMaps(response.Items, &objects)
			if err != nil {
				log.Printf("Couldn't unmarshal query response. Here's why: %v\n", err)
			}
		}
	}
	return objects, err
}

func (basics TableBasics) UpdateName(object string, name string) (map[string]map[string]interface{}, error) {
	var err error
	var response *dynamodb.UpdateItemOutput
	var attributeMap map[string]map[string]interface{}
	update := expression.Set(expression.Name("name"), expression.Value(name))
	expr, err := expression.NewBuilder().WithUpdate(update).Build()
	if err != nil {
		log.Printf("Couldn't build expression for update. Here's why: %v\n", err)
	} else {
		response, err = basics.DynamoDbClient.UpdateItem(context.TODO(), &dynamodb.UpdateItemInput{
			TableName:                 aws.String(basics.TableName),
			Key:                       map[string]types.AttributeValue{
        									"object": &types.AttributeValueMemberS{Value: object},
										},
			ExpressionAttributeNames:  expr.Names(),
			ExpressionAttributeValues: expr.Values(),
			UpdateExpression:          expr.Update(),
			ReturnValues:              types.ReturnValueUpdatedNew,
		})
		if err != nil {
			log.Printf("Couldn't update record with pk %v. Here's why: %v\n", object, err)
		} else {
			err = attributevalue.UnmarshalMap(response.Attributes, &attributeMap)
			if err != nil {
				log.Printf("Couldn't unmarshall update response. Here's why: %v\n", err)
			}
		}
	}
	return attributeMap, err
}

func handleResponse(rw http.ResponseWriter, err error) {
	rw.WriteHeader(200)
	if err != nil {
		log.Printf("Error: %s", err)
	}
}

func Handle(rw http.ResponseWriter, req *http.Request) {
	fmt.Println("request", req)
    gw := os.Getenv("GATEWAY")
    findKey := "/find "
    tableName := "main"
    
    customResolver := aws.EndpointResolverWithOptionsFunc(func(service, region string, options ...interface{}) (aws.Endpoint, error) {
        if service == dynamodb.ServiceID && region == "ru-central1" {
            return aws.Endpoint{
				PartitionID:   "yc",
                URL:           os.Getenv("ENDPOINT_URL_DB"),
                SigningRegion: "ru-central1",
            }, nil
        }
        return aws.Endpoint{}, fmt.Errorf("unknown endpoint requested")
    })
    
    cfg, err := config.LoadDefaultConfig(context.TODO(), config.WithEndpointResolverWithOptions(customResolver))
	if err != nil {
		log.Fatal(err)
	}
    
//     sdk, err := ycsdk.Build(ctx, ycsdk.Config{
//         Credentials: ycsdk.InstanceServiceAccount(),
//     })
//     if err != nil {
//         return nil, err
//     }
    
	bot, err := tgbotapi.NewBotAPI(os.Getenv("TLG_API"))
	if err != nil {
		log.Fatal(err)
	}

	bot.Debug = true

	log.Printf("Authorized on account %s", bot.Self.UserName)

	update, err := parseTelegramRequest(req)
	if err != nil {
		log.Printf("error parsing update, %s", err.Error())
		// return
	} else {
        log.Printf("[?] %s", update.Message.Text)
        
        var msgs []tgbotapi.Chattable
        tableBasics := TableBasics{dynamodb.NewFromConfig(cfg), tableName}
//             tables, err := tableBasics.ListTables()
//             if err == nil {
//                 for i := range tables {
//                     log.Printf("%s", tables[i])
//                 }
//             }
        if update.Message.Text == "/getface" {
            exists, err := tableBasics.TableExists()
            if err == nil && exists {
                objects, err := tableBasics.QueryNullName()
                log.Println("objects", objects)
                if err == nil && len(objects) > 0 {
                    log.Println("objects[0]", objects[0])
                    log.Println("objects[0].object", objects[0].Object)
                    log.Println("objects[0].name", objects[0].Name)
                    msgs = append(msgs, tgbotapi.NewPhoto(update.Message.Chat.ID, tgbotapi.FileURL(gw + "/?face=" + objects[0].Object)))
                } else {
                    msgs = append(msgs, tgbotapi.NewMessage(update.Message.Chat.ID, "Фотографии без имени не найдены"))
                }
            } else {
                msgs = append(msgs, tgbotapi.NewMessage(update.Message.Chat.ID, "Фотографии без имени не найдены"))
            }
        } else if strings.HasPrefix(update.Message.Text, findKey) { // "/find "
            name := strings.TrimPrefix(update.Message.Text, findKey)
            exists, err := tableBasics.TableExists()
            if err == nil && exists {
                objects, err := tableBasics.Query(name)
                if err == nil && len(objects) > 0 {
                    for i := range objects {
                        msgs = append(msgs, tgbotapi.NewPhoto(update.Message.Chat.ID, tgbotapi.FileURL(gw + "/?face=" + objects[i].Object)))
                    }
                } else {
                    msgs = append(msgs, tgbotapi.NewMessage(update.Message.Chat.ID, "Фотографии с " + name + " не найдены"))
                }
            } else {
                msgs = append(msgs, tgbotapi.NewMessage(update.Message.Chat.ID, "Фотографии с " + name + " не найдены"))
            }
        } else {
            exists, err := tableBasics.TableExists()
            if err == nil && exists {
                objects, err := tableBasics.QueryNullName()
                log.Println("objects", objects)
                if err == nil && len(objects) > 0 {
                    _, err := tableBasics.UpdateName(objects[0].Object, update.Message.Text)
                    if err != nil {
                        msgs = append(msgs, tgbotapi.NewMessage(update.Message.Chat.ID, "Фотографии без имени не найдены"))
                    } else {
                        msgs = append(msgs, tgbotapi.NewMessage(update.Message.Chat.ID, update.Message.Text))
                    }
                } else {
                    msgs = append(msgs, tgbotapi.NewMessage(update.Message.Chat.ID, "Фотографии без имени не найдены"))
                }
            } else {
                msgs = append(msgs, tgbotapi.NewMessage(update.Message.Chat.ID, "Фотографии без имени не найдены"))
            }
        }
        
        for i := range msgs {
            bot.Send(msgs[i])
        }
    }
	handleResponse(rw, err)
}

func (basics TableBasics) ListTables() ([]string, error) {
	var tableNames []string
	tables, err := basics.DynamoDbClient.ListTables(
		context.TODO(), &dynamodb.ListTablesInput{})
	if err != nil {
		log.Printf("Couldn't list tables. Here's why: %v\n", err)
	} else {
		tableNames = tables.TableNames
	}
	return tableNames, err
}
