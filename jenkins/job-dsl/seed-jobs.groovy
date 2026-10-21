// Job DSL: every job is code. Applied by JCasC on startup.
folder('nmc') { description('Network Management Card firmware') }

multibranchPipelineJob('nmc/nmc-firmware') {
  displayName('NMC firmware - CI')
  branchSources {
    github {
      id('nmc-firmware')
      repoOwner('lohith-rn06'); repository('nmc-firmware-devops')
      scanCredentialsId('github-app')
    }
  }
  factory { workflowBranchProjectFactory { scriptPath('jenkins/Jenkinsfile') } }
  orphanedItemStrategy { discardOldItems { daysToKeep(7) } }
  triggers { periodicFolderTrigger { interval('1d') } }   // webhooks do the real triggering
}

multibranchPipelineJob('nmc/nmc-firmware-release') {
  displayName('NMC firmware - RELEASE (tags)')
  branchSources {
    branchSource {
      source {
        github {
          id('nmc-release'); repoOwner('lohith-rn06'); repository('nmc-firmware-devops')
          credentialsId('github-app'); repositoryUrl('https://github.com/lohith-rn06/nmc-firmware-devops')
          traits { gitHubTagDiscovery() }
        }
      }
      buildStrategies { buildTags { atLeastDays('-1'); atMostDays('3') } }
    }
  }
  factory { workflowBranchProjectFactory { scriptPath('jenkins/Jenkinsfile.release') } }
}

['nightly': 'jenkins/Jenkinsfile.nightly', 'toolchain-image': 'jenkins/Jenkinsfile.toolchain'].each { name, path ->
  pipelineJob("nmc/${name}") {
    definition {
      cpsScm {
        scm { git { remote { url('https://github.com/lohith-rn06/nmc-firmware-devops.git'); credentials('github-app') }; branch('main') } }
        scriptPath(path)
      }
    }
  }
}
