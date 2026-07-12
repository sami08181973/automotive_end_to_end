# Azure free-tier skeleton – ACR + Container Instances / AKS
terraform {
  required_version = ">= 1.5.0"
  required_providers {
    azurerm = {
      source  = "hashicorp/azurerm"
      version = "~> 3.0"
    }
  }
}

provider "azurerm" {
  features {}
}

variable "location" {
  type    = string
  default = "eastus"
}

variable "prefix" {
  type    = string
  default = "sdvvecu"
}

resource "azurerm_resource_group" "rg" {
  name     = "${var.prefix}-rg"
  location = var.location
}

resource "azurerm_container_registry" "acr" {
  name                = "${var.prefix}acr"
  resource_group_name = azurerm_resource_group.rg.name
  location            = azurerm_resource_group.rg.location
  sku                 = "Basic"
  admin_enabled       = true
}

output "acr_login_server" {
  value = azurerm_container_registry.acr.login_server
}

output "next_steps" {
  value = <<-EOT
    1) az acr login --name ${azurerm_container_registry.acr.name}
    2) docker build -f cloud/docker/Dockerfile -t ${azurerm_container_registry.acr.login_server}/sdv:latest .
    3) docker push ${azurerm_container_registry.acr.login_server}/sdv:latest
    4) az container create ... OR AKS apply cloud/kubernetes/sdv-jobs.yaml
    Free: Azure free account + student credits; use CPU for emulation, NC-series for GPU.
  EOT
}
