data "aws_iam_policy_document" "eks_assume" {
  statement {
    actions = ["sts:AssumeRole"]
    principals {
      type        = "Service"
      identifiers = ["eks.amazonaws.com"]
    }
  }
}

data "aws_iam_policy_document" "node_assume" {
  statement {
    actions = ["sts:AssumeRole"]
    principals {
      type        = "Service"
      identifiers = ["ec2.amazonaws.com"]
    }
  }
}

resource "aws_iam_role" "cluster" {
  name               = "${var.name}-cluster"
  assume_role_policy = data.aws_iam_policy_document.eks_assume.json
}

resource "aws_iam_role_policy_attachment" "cluster" {
  role       = aws_iam_role.cluster.name
  policy_arn = "arn:aws:iam::aws:policy/AmazonEKSClusterPolicy"
}

resource "aws_kms_key" "secrets" {
  description         = "${var.name} EKS secrets envelope encryption"
  enable_key_rotation = true
}

resource "aws_eks_cluster" "this" {
  name     = var.name
  version  = var.k8s_version
  role_arn = aws_iam_role.cluster.arn
  vpc_config {
    subnet_ids              = var.subnet_ids
    endpoint_private_access = true
    endpoint_public_access  = var.public_endpoint
    public_access_cidrs     = var.admin_cidrs
  }
  encryption_config {
    resources = ["secrets"]
    provider {
      key_arn = aws_kms_key.secrets.arn
    }
  }
  access_config {
    authentication_mode = "API"
  }
  enabled_cluster_log_types = ["api", "audit", "authenticator"]
  depends_on                = [aws_iam_role_policy_attachment.cluster]
}

resource "aws_iam_role" "node" {
  name               = "${var.name}-node"
  assume_role_policy = data.aws_iam_policy_document.node_assume.json
}

resource "aws_iam_role_policy_attachment" "node" {
  for_each   = toset(["AmazonEKSWorkerNodePolicy", "AmazonEKS_CNI_Policy", "AmazonEC2ContainerRegistryReadOnly", "AmazonSSMManagedInstanceCore", "service-role/AmazonEBSCSIDriverPolicy"])
  role       = aws_iam_role.node.name
  policy_arn = "arn:aws:iam::aws:policy/${each.value}"
}

# System pool: Jenkins controller, SonarQube, lab simulators (on-demand, stable)
resource "aws_eks_node_group" "system" {
  cluster_name    = aws_eks_cluster.this.name
  node_group_name = "system"
  node_role_arn   = aws_iam_role.node.arn
  subnet_ids      = var.subnet_ids
  instance_types  = ["m6i.large"]
  scaling_config {
    min_size     = 2
    max_size     = 4
    desired_size = 2
  }
  labels = { "nmc.io/pool" = "system" }
}

# Build-agent pool: Jenkins agent pods only (Spot, scales to zero overnight)
resource "aws_eks_node_group" "build_agents" {
  cluster_name    = aws_eks_cluster.this.name
  node_group_name = "build-agents"
  node_role_arn   = aws_iam_role.node.arn
  subnet_ids      = var.subnet_ids
  capacity_type   = "SPOT"
  instance_types  = ["c6i.2xlarge", "c6a.2xlarge", "c5.2xlarge"]
  scaling_config {
    min_size     = 0
    max_size     = var.max_build_nodes
    desired_size = 1
  }
  labels = { "nmc.io/pool" = "build-agents" }
  taint {
    key    = "nmc.io/build-agents"
    value  = "true"
    effect = "NO_SCHEDULE"
  }
  lifecycle {
    ignore_changes = [scaling_config[0].desired_size]
  }
}

data "tls_certificate" "oidc" {
  url = aws_eks_cluster.this.identity[0].oidc[0].issuer
}

resource "aws_iam_openid_connect_provider" "this" {
  url             = aws_eks_cluster.this.identity[0].oidc[0].issuer
  client_id_list  = ["sts.amazonaws.com"]
  thumbprint_list = [data.tls_certificate.oidc.certificates[0].sha1_fingerprint]
}

resource "aws_eks_addon" "this" {
  for_each     = toset(["vpc-cni", "coredns", "kube-proxy", "aws-ebs-csi-driver"])
  cluster_name = aws_eks_cluster.this.name
  addon_name   = each.value
  depends_on   = [aws_eks_node_group.system]
}

resource "aws_eks_access_entry" "admins" {
  for_each      = toset(var.admin_role_arns)
  cluster_name  = aws_eks_cluster.this.name
  principal_arn = each.value
}

resource "aws_eks_access_policy_association" "admins" {
  for_each      = toset(var.admin_role_arns)
  cluster_name  = aws_eks_cluster.this.name
  principal_arn = each.value
  policy_arn    = "arn:aws:eks::aws:cluster-access-policy/AmazonEKSClusterAdminPolicy"
  access_scope {
    type = "cluster"
  }
}
