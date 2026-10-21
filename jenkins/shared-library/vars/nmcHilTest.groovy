// Hardware-in-the-loop: flash real NMC cards in the lab rack with Ansible, then run the
// same integration suite against the physical card. Runs on a lab agent (label 'hil').
def call(Map args) {
  node('hil') {
    unstash 'fw-GVS'; args.variants.findAll { it != 'GVS' }.each { unstash "fw-${it}" }
    withCredentials([usernamePassword(credentialsId: 'hil-card-admin', usernameVariable: 'NMC_ADMIN_USER', passwordVariable: 'NMC_ADMIN_PASS')]) {
      sh 'ansible-playbook -i ansible/inventories/hil-lab.yml ansible/playbooks/flash-firmware.yml -e fw_dir=$WORKSPACE/dist'
      sh 'ansible-playbook -i ansible/inventories/hil-lab.yml ansible/playbooks/verify-cards.yml'
    }
  }
}
