// Helpers to retrieve information from GitHub when the website is built.
// The requests are optional: if they fail (offline, rate limit...), the pages fall back to static links.

import vampPlugins from '../assets/generated/vamp-plugins.json';

export const repositoryUrl = 'https://github.com/Ircam-Partiels/Partiels';
export const pluginsRepositoryUrl = 'https://github.com/Ircam-Partiels/Partiels-Vamp-Plugins';
export const discussionsUrl = 'https://github.com/orgs/Ircam-Partiels/discussions';

// The release of the Partiels Vamp Plugins associated with this version of Partiels (PARTIELS_VAMP_PLUGINS_TAG in
// CMakeLists.txt, retrieved by scripts/sync.mjs)
export const pluginsTag: string = vampPlugins.tag;
export const pluginsReleaseUrl = `${pluginsRepositoryUrl}/releases/tag/${pluginsTag}`;

export function pluginsDownloadUrl(file: string): string {
	return `${pluginsRepositoryUrl}/releases/download/${pluginsTag}/${file}`;
}

const cache = new Map<string, Promise<unknown>>();

async function fetchJson<T>(url: string, useToken: boolean): Promise<T | null> {
	const headers: Record<string, string> = { Accept: 'application/json' };
	const token = process.env.GITHUB_TOKEN;
	if (useToken && token) {
		headers.Authorization = `Bearer ${token}`;
	}
	try {
		const response = await fetch(url, { headers });
		if (!response.ok) {
			throw new Error(`${response.status} ${response.statusText}`);
		}
		return (await response.json()) as T;
	} catch (error) {
		console.warn(`[github] cannot fetch ${url}: ${error}`);
		return null;
	}
}

function cached<T>(url: string, useToken: boolean): Promise<T | null> {
	if (!cache.has(url)) {
		cache.set(url, fetchJson<T>(url, useToken));
	}
	return cache.get(url) as Promise<T | null>;
}

export interface Release {
	tag_name: string;
	html_url: string;
	published_at: string;
}

export function getLatestRelease(): Promise<Release | null> {
	return cached<Release>('https://api.github.com/repos/Ircam-Partiels/Partiels/releases/latest', true);
}

export interface Plugin {
	identifier: string;
	name: string;
	maker: string;
	version: number;
	libraryDescription: string;
	pluginDescription: string;
	minVersionMacos?: string;
	architecturesMacos?: string[];
	downloadUrlMacos?: string;
	downloadUrlWindows?: string;
	downloadUrlLinux?: string;
}

export interface PluginDistribution {
	minimumPartielsVersion?: string;
	plugins: Plugin[];
}

export async function getPlugins(): Promise<PluginDistribution | null> {
	// Release asset: the token is not required (and the redirection to the storage server must not receive it)
	const distribution = await cached<Plugin[] | PluginDistribution>(pluginsDownloadUrl('Partiels-Plugins.json'), false);
	// The list is either an object with the minimum version of Partiels required by the plugins and the list of plugins,
	// or only the list of plugins (the legacy format that is compatible with all the versions of Partiels)
	if (Array.isArray(distribution)) {
		return { plugins: distribution };
	}
	return distribution && Array.isArray(distribution.plugins) ? distribution : null;
}
