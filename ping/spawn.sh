set -ex
# Create an SSH keypair
rm -f ~/.ssh/dpdk.pem
aws ec2 create-key-pair \
    --key-name dpdk \
    --query KeyMaterial \
    --output text > ~/.ssh/dpdk.pem
[ -s ~/.ssh/dpdk.pem ] || { echo "Failed to write ~/.ssh/dpdk.pem" >&2; exit 1; }
echo "SSH keypair created: ~/.ssh/dpdk.pem" >&2

# Set correct permissions for the key
chmod 400 ~/.ssh/dpdk.pem

# Create security group and store into SG_ID
SG_ID=$(aws ec2 create-security-group \
  --group-name dpdk \
  --description "DPDK" \
  --query GroupId \
  --output text)
echo "Security group created: $SG_ID" >&2

# Allow all inbound traffic for IPv4 and IPv6. Outbounds allowed by default
RULE_ID=$(aws ec2 authorize-security-group-ingress \
  --group-id $SG_ID \
  --ip-permissions \
    'IpProtocol=-1,IpRanges=[{CidrIp=0.0.0.0/0}],Ipv6Ranges=[{CidrIpv6=::/0}]' \
  --query 'SecurityGroupRules[0].SecurityGroupRuleId' \
  --output text)
echo "Ingress rule created: $RULE_ID" >&2

# Get default subnet
DEFAULT_SUBNET=$(aws ec2 describe-subnets \
  --filters "Name=defaultForAz,Values=true" \
  --query 'Subnets[0].SubnetId' \
  --output text)
echo "Default subnet: $DEFAULT_SUBNET" >&2

# Creates a network card with a public IPv4 address
# Only network cards with public IPs can access the internet
create_nic_with_eip() {
  local NIC_ID=$(aws ec2 create-network-interface \
    --subnet-id $DEFAULT_SUBNET \
    --groups $SG_ID \
    --query 'NetworkInterface.NetworkInterfaceId' \
    --output text)
  echo "Network interface created: $NIC_ID" >&2

  local EIP_ALLOC_ID=$(aws ec2 allocate-address \
    --domain vpc \
    --query 'AllocationId' \
    --output text)
  echo "Elastic IP allocated: $EIP_ALLOC_ID" >&2

  local EIP_ASSOC_ID=$(aws ec2 associate-address \
    --allocation-id $EIP_ALLOC_ID \
    --network-interface-id $NIC_ID \
    --query 'AssociationId' \
    --output text)
  echo "Elastic IP associated: $EIP_ASSOC_ID" >&2

  echo "$NIC_ID"
}

# Create two network cards for the EC2
NIC0_ID=$(create_nic_with_eip)
echo "NIC0 configured: $NIC0_ID" >&2
NIC1_ID=$(create_nic_with_eip)
echo "NIC1 configured: $NIC1_ID" >&2

# Spawn our development EC2
# The instance runs Ubuntu 26.04 LTS in us-east-1
INSTANCE_ID=$(aws ec2 run-instances \
  --key-name dpdk \
  --region us-east-1 \
  --image-id ami-091138d0f0d41ff90 \
  --instance-type t3.small \
  --network-interfaces \
    NetworkInterfaceId=$NIC0_ID,DeviceIndex=0 \
    NetworkInterfaceId=$NIC1_ID,DeviceIndex=1 \
  --query 'Instances[0].InstanceId' \
  --output text)
echo "EC2 instance launched: $INSTANCE_ID" >&2

# Get the public IP of nic0 to use for SSH
NIC0_PUBLIC_IP=$(aws ec2 describe-network-interfaces \
  --network-interface-ids $NIC0_ID \
  --query 'NetworkInterfaces[0].Association.PublicIp' \
  --output text)
echo "nic0 public IP: $NIC0_PUBLIC_IP" >&2

echo "Waiting for instance to be running..." >&2
aws ec2 wait instance-running --instance-ids $INSTANCE_ID

echo "Setup complete. Run ssh -i ~/.ssh/dpdk.pem ubuntu@$NIC0_PUBLIC_IP to connect to the instance." >&2
