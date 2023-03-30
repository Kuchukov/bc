sudo apt install nlohmann-json3-dev libcurl4-openssl-dev

g++ -std=c++11 -I/usr/include -I/usr/include/nlohmann -o bc bc.cpp -lcurl

./bc --url http://192.168.1.7:8081 --address "bcnZKxVKM3aaa5qGKos5UVK4KP3eEny2QYZqWzLWktEr2CSEAQAJmnU11r3ms7q4vS5uEoXkGyvC6PqnX2MqrWB663MdpNCaQs" --user "user" --password "password"
