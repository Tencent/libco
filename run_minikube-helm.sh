#!/usr/bin/env sh


minikube start --force
# create dedicated namespace where Jenkins helm chart will be installed
minikube kubectl create namespace jenkins
# install helm
wget https://get.helm.sh/helm-v3.9.2-linux-amd64.tar.gz
tar -zxvf helm-v3.9.2-linux-amd64.tar.gz
mv linux-amd64/helm /usr/local/bin/helm
install jenkins chart
helm repo add jenkinsci https://charts.jenkins.io
helm repo update
helm install jenkins -n jenkins jenkinsci/jenkins

#minikube kubectl -- get all -n jenkins
echo "Waiting till jenkins deployment is ready"

while true; do
    status=`minikube kubectl -- get all -n jenkins | grep "statefulset.apps/jenkins" | awk ' { print $2 }' | sed -e 's/\// /' | awk '{ print $1 }'```
    if [ "$status" = "1" ]; then
        break
    fi
done
echo "Running tests"
helm test jenkins -n jenkins
