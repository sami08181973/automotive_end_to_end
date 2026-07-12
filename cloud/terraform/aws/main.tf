# AWS free-tier oriented skeleton – Virtual ECU on ECS Fargate / EKS
# Requires: terraform, AWS CLI credentials (aws configure)
terraform {
  required_version = ">= 1.5.0"
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
  }
}

provider "aws" {
  region = var.aws_region
}

variable "aws_region" {
  type    = string
  default = "us-east-1"
}

variable "project" {
  type    = string
  default = "sdv-virtual-ecu"
}

# ECR repository to host the Virtual ECU container image
resource "aws_ecr_repository" "sdv" {
  name                 = var.project
  image_tag_mutability = "MUTABLE"
  force_delete         = true
}

output "ecr_repository_url" {
  value       = aws_ecr_repository.sdv.repository_url
  description = "Push: docker tag embedailabs/sdv-virtual-ecu:latest <url>:latest && docker push"
}

output "next_steps" {
  value = <<-EOT
    1) aws ecr get-login-password --region ${var.aws_region} | docker login --username AWS --password-stdin ${aws_ecr_repository.sdv.repository_url}
    2) docker build -f cloud/docker/Dockerfile -t ${aws_ecr_repository.sdv.repository_url}:latest .
    3) docker push ${aws_ecr_repository.sdv.repository_url}:latest
    4) Run on ECS Fargate free-tier eligible task OR EKS/Fargate with cloud/kubernetes/sdv-jobs.yaml
    Free options: AWS Free Tier, LocalStack for local emul, Graviton ARM instances for ARM backend.
  EOT
}
