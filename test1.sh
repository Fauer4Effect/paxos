# sudo docker build -t dist_docker .
# sudo docker network create --subnet "192.168.69.0\24" mynet

sudo docker run \
    --net mynet \
    --ip "192.168.69.5" \
    --hostname foo.bar.1 \
    --add-host foo.bar.2:192.168.69.6 \
    --add-host foo.bar.3:192.168.69.7 \
    --add-host foo.bar.4:192.168.69.8 \
    --name cont1 \
    -a stdout -a stderr -a stdin \
    dist_docker \
    ./paxos -p 55555 -h hosts_local &

sleep 5

sudo docker container kill $(sudo docker ps -q | cut -d " " -f 1)
sudo docker container prune -f

