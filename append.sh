precmd() {
		RPROMPT=$(~/.powerline 1 $?)
		PROMPT=$(~/.powerline 0)
}
