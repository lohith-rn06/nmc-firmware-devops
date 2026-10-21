# Jenkins controller for LOCAL practice (phase 3 of the guide). Plugins + JCasC baked in.
FROM jenkins/jenkins:2.479.1-lts-jdk17
USER root
RUN apt-get update && apt-get install -y --no-install-recommends docker.io && rm -rf /var/lib/apt/lists/*
USER jenkins
ENV JAVA_OPTS="-Djenkins.install.runSetupWizard=false" CASC_JENKINS_CONFIG=/var/jenkins_home/casc
COPY --chown=jenkins:jenkins plugins.txt /usr/share/jenkins/ref/plugins.txt
RUN jenkins-plugin-cli -f /usr/share/jenkins/ref/plugins.txt
COPY --chown=jenkins:jenkins casc-local.yaml /var/jenkins_home/casc/jenkins.yaml
