{{- define "nmc.name" -}}nmc-{{ .Values.variant | lower }}{{- end }}
{{- define "nmc.labels" -}}
app.kubernetes.io/name: nmc-simulator
app.kubernetes.io/instance: {{ include "nmc.name" . }}
nmc.io/variant: {{ .Values.variant | quote }}
helm.sh/chart: {{ .Chart.Name }}-{{ .Chart.Version }}
{{- end }}
