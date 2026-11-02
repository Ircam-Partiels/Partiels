// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

// The URL of the website is provided by the GitHub Pages workflow (which also handles custom domains),
// the default values correspond to https://ircam-partiels.github.io/Partiels.
export default defineConfig({
	site: process.env.SITE_URL || 'https://ircam-partiels.github.io',
	base: process.env.BASE_PATH ?? '/Partiels',
	integrations: [
		starlight({
			title: 'Partiels',
			description: 'Partiels is a software suite for analysing and exploring the content and characteristics of sounds.',
			logo: { src: './src/assets/generated/icon.png' },
			favicon: '/media/icon.png',
			customCss: ['./src/styles/custom.css'],
			components: { Hero: './src/components/Hero.astro' },
			social: [{ icon: 'github', label: 'GitHub', href: 'https://github.com/Ircam-Partiels/Partiels' }],
			editLink: { baseUrl: 'https://github.com/Ircam-Partiels/Partiels/edit/main/Website/' },
			lastUpdated: true,
			sidebar: [
				{ label: 'Home', link: '/' },
				{
					label: 'Get Started',
					items: [
						{ label: 'Download', slug: 'download' },
						{ label: 'Vamp Plugins', slug: 'plugins' },
						{ label: 'Manual', slug: 'manual' },
					],
				},
				{
					label: 'Resources',
					items: [
						{ label: 'Videos', slug: 'resources/videos' },
						{ label: 'Publications', slug: 'resources/publications' },
					],
				},
				{
					label: 'Community',
					items: [
						{ label: 'Support and Feedback', slug: 'support' },
						{ label: 'About', slug: 'about' },
					],
				},
			],
		}),
	],
});
