precmd() {
		RPROMPT=$(~/scripts/prompt.exe 1 $?)
		PROMPT=$(~/scripts/prompt.exe 0)
}
