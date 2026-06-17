node_type = "m3"
nodes20 = ("m3-145" "m3-152" "m3-153" "m3-155" "m3-157" "m3-163" "m3-164" "m3-171" "m3-173" "m3-180" "m3-181" "m3-192" "m3-194" "m3-195" "m3-199" "m3-200" "m3-202" "m3-210" "m3-222")
site = ".lille"
end_node = ".iot-lab.info"

for node_id in $nodes20
do
    IOTLAB_NODE=$node_id$site$end_node BOARD=$node_type make all flash
done