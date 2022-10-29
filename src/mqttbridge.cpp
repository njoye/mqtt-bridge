#include "ros/ros.h"
#include <std_msgs/UInt32.h>
#include "easywsclient.hpp"
#include "easywsclient.cpp"
#include "PreferencesLoader.h"
#include "mqtt/async_client.h"
#include <unordered_map>

Json::Value root;
Json::Reader reader;
std::unordered_map<std::string, std::string> rosMqttTopicMap;
mqtt::async_client cli("127.0.0.1", "");

// Will only be executed if a message is present
void handle_message(const std::string & message)
{
    bool success = reader.parse( message, root );
    
    if(success && root.isMember("op")){
        std::string op = root["op"].asString();
        if(op == "publish" && root.isMember("topic")){
            std::string rosTopic = root["topic"].asString();
            // Check if topic exists in mapping
            if(rosMqttTopicMap.find(rosTopic) != rosMqttTopicMap.end()){                
                // Check if msg is present (should be)
                if(root.isMember("msg")){
                    mqtt::topic mqttTopic(cli, rosMqttTopicMap[rosTopic], 0);

                    // Publish value and wait for publishing to finish
                    mqtt::token_ptr tok;
                    tok = mqttTopic.publish(root["msg"].toStyledString());
                    //tok->wait();
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    ros::init(argc, argv, "talker");
    ros::NodeHandle n;

    std::string mqttBroker = "127.0.0.1";
    int mqttPort = 1883;

    std::string rosbridgeAddress = "127.0.0.1";
    int rosbridgePort = 1884;

	try {
        // Connect to MQTT Broker
        ROS_INFO_STREAM("Connecting to MQTT Broker at" << mqttBroker << ":" << mqttPort);
		cli.connect()->wait();
        ROS_INFO_STREAM("Successfully connected to MQTT Broker at" << mqttBroker << ":" << mqttPort);
	}
	catch (const mqtt::exception& exc) {
		ROS_ERROR_STREAM("Couldn't connect to MQTT Broker!");
		return 1;
	}
        
    // Connect to ROSBridge_Server
    ROS_INFO_STREAM("Connecting to rosbridge_server at " << rosbridgeAddress << ":" << rosbridgePort);
    easywsclient::WebSocket::pointer ws = easywsclient::WebSocket::from_url("ws://127.0.0.1:1884");
    assert(ws);
    ROS_INFO_STREAM("Successfully connected to rosbridge_server at " << rosbridgeAddress << ":" << rosbridgePort);
    
    rosMqttTopicMap["/ros/test"] = "/ros/test";

    for(const auto &pair : rosMqttTopicMap){
        ROS_INFO_STREAM("Subscribing to ROS topic '" << pair.first << "' and publishing it on MQTT topic '" << pair.second << "'");
        ws->send("{\"op\": \"subscribe\", \"topic\": \"/ros/test\"}");
        ws->poll();
    }

    while(ros::ok()){
        ws->poll();
        ws->dispatch(handle_message);
    }

    ROS_ERROR("Shutdown received, will terminate.");
    ROS_WARN("Disconnecting from MQTT Broker.");
    cli.disconnect()->wait();

    ROS_WARN("Disconnecting from rosbridge_server.");
    delete ws;

    return 0;
}