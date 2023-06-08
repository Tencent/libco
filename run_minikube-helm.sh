#!/usr/bin/env sh


minikube start --force

helm create new-chart
helm install new-chart ./new-chart
