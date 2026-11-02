// Imports the content shared with the rest of the repository (manual, images, icon) into the website.
// The generated files are ignored by git (see ../.gitignore): Docs/ and BinaryData/ remain the single sources.

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const websiteDir = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const repositoryDir = path.resolve(websiteDir, '..');
const docsDir = path.join(repositoryDir, 'Docs');
const resourceDir = path.join(repositoryDir, 'BinaryData', 'Resource');
const mediaDir = path.join(websiteDir, 'public', 'media');
const assetsDir = path.join(websiteDir, 'src', 'assets', 'generated');
const manualFile = path.join(websiteDir, 'src', 'content', 'docs', 'manual.md');

const resources = ['CP-LOGO-IRCAM-noir.svg', 'CP-LOGO-IRCAM-blanc.svg', 'icon.png'];

function copyFile(source, destination) {
	fs.mkdirSync(path.dirname(destination), { recursive: true });
	fs.copyFileSync(source, destination);
}

fs.rmSync(mediaDir, { recursive: true, force: true });
fs.rmSync(assetsDir, { recursive: true, force: true });

// Images of the manual and shared resources (served from public/media)
fs.cpSync(path.join(docsDir, 'Images'), path.join(mediaDir, 'manual'), { recursive: true });
for (const resource of resources) {
	copyFile(path.join(resourceDir, resource), path.join(mediaDir, resource));
}
// Images processed by Astro (with hashed URLs that do not depend on the base path)
for (const resource of ['icon.png', 'Partiels-v2.0.0-Sample-v2.gif']) {
	copyFile(path.join(resourceDir, resource), path.join(assetsDir, resource));
}

// Manual: the title page and the table of contents are replaced by the page title and the sidebar of the website,
// the page breaks are only meaningful for the PDF, and the image paths are relative to the /manual/ page.
const manual = fs.readFileSync(path.join(docsDir, 'Partiels-Manual.md'), 'utf8');
const start = manual.search(/^## 1\. /m);
if (start < 0) {
	throw new Error('The first section of the manual cannot be found in Docs/Partiels-Manual.md');
}
const body = manual
	.slice(start)
	.replace(/^<div style="page-break-after: always;"><\/div>\s*$/gm, '')
	.replace(/src="Images\//g, 'src="../media/manual/')
	.replace(/src="\.\.\/BinaryData\/Resource\/([^"]+)"/g, (_, file) => {
		copyFile(path.join(resourceDir, file), path.join(mediaDir, file));
		return `src="../media/${file}"`;
	});

// The note on the versions of the manual is displayed as a banner (Markdown directives are not reliably processed
// by the development server for this generated file).
const banner =
	'This online manual follows the development version of Partiels. The manual of each release is also available as a PDF file: ' +
	'<a href="https://github.com/Ircam-Partiels/Partiels/releases/latest/download/Partiels-Manual.pdf">Partiels-Manual.pdf</a>.';
const header = `---
title: Manual
description: User manual of Partiels, the software for analysing and exploring the content and characteristics of sounds.
editUrl: https://github.com/Ircam-Partiels/Partiels/edit/main/Docs/Partiels-Manual.md
banner:
  content: ${JSON.stringify(banner)}
---

`;

fs.writeFileSync(manualFile, header + body);
console.log(`Website synchronized with ${path.relative(process.cwd(), repositoryDir) || '.'}`);
