#!/usr/bin/env sh


minikube start --force

pwd
ls
echo "======================= Running helm ==========================="
helm install my-demo-chart demo-chart/ --values demo-chart/values.yaml

echo "==================="
echo "Nodes:"
kubectl get nodes
echo "Pods:"
kubectl get pods
echo "Services:"
kubectl get services

echo "==================="
export NODE_PORT=$(kubectl get --namespace default -o jsonpath="{.spec.ports[0].nodePort}" services my-demo-chart)
export NODE_IP=$(kubectl get nodes --namespace default -o jsonpath="{.items[0].status.addresses[0].address}")
echo http://$NODE_IP:$NODE_PORT

echo "waiting 30 seconds for nginx to come up properly.."
sleep 30
echo "Running curl now..."
curl -i http://$NODE_IP:$NODE_PORT
ret=$?
echo "=================================="
if [ "$ret" != "0" ]
then
	echo "NGINX Connection successful."
else
	echo "NGINX Connection unsuccesful."
fi
exit $ret
