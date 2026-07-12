# GCP free-tier / Colab-adjacent skeleton – Artifact Registry + Cloud Run Job
terraform {
  required_version = ">= 1.5.0"
  required_providers {
    google = {
      source  = "hashicorp/google"
      version = "~> 5.0"
    }
  }
}

provider "google" {
  project = var.project_id
  region  = var.region
}

variable "project_id" {
  type        = string
  description = "GCP project id (create free trial at https://console.cloud.google.com)"
}

variable "region" {
  type    = string
  default = "us-central1"
}

resource "google_artifact_registry_repository" "sdv" {
  location      = var.region
  repository_id = "sdv-virtual-ecu"
  format        = "DOCKER"
}

output "registry" {
  value = "${var.region}-docker.pkg.dev/${var.project_id}/sdv-virtual-ecu"
}

output "next_steps" {
  value = <<-EOT
    Free paths:
      A) Google Colab (free NVIDIA T4): open cloud/colab/sdv_virtual_ecu.ipynb
      B) GCP Free Trial / Always Free:
         gcloud auth configure-docker ${var.region}-docker.pkg.dev
         docker build -f cloud/docker/Dockerfile -t ${var.region}-docker.pkg.dev/${var.project_id}/sdv-virtual-ecu/sdv:latest .
         docker push ...
         gcloud run jobs execute ... OR GKE Autopilot apply cloud/kubernetes/sdv-jobs.yaml
  EOT
}
