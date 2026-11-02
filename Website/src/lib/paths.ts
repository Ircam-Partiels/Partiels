// Returns the URL of a file of the public directory, taking into account the base path of the website.
export function withBase(file: string): string {
	return `${import.meta.env.BASE_URL.replace(/\/$/, '')}/${file.replace(/^\//, '')}`;
}
