package org.pescuma.miricogen;

import static java.lang.Math.*;
import static org.pescuma.miricogen.FileUtils.*;
import static org.pescuma.miricogen.ImageUtils.*;

import java.awt.image.BufferedImage;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileFilter;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Properties;
import java.util.Set;
import java.util.TreeSet;

import net.sf.image4j.codec.ico.ICOEncoder;

import org.apache.tools.ant.taskdefs.Copy;
import org.apache.tools.ant.taskdefs.ExecTask;
import org.apache.velocity.Template;
import org.apache.velocity.VelocityContext;
import org.apache.velocity.app.Velocity;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.pescuma.jfg.gui.swt.JfgFormComposite;
import org.pescuma.jfg.gui.swt.JfgFormData;
import org.pescuma.jfg.reflect.ReflectionGroup;
import org.pescuma.velocity.VelocityLogger;
import org.pescuma.velocity.VelocityResourceLoader;

public class Main
{
	private static Config cfg;
	private static Text out;
	
	public static class Paths
	{
		public File protosPath = new File("protos");
		public File overlaysPath = new File("overlays");
		public File pluginsPath = new File("plugins");
		public File outPath = new File("out");
	}
	
	public static class DllInfo
	{
		public String name = "Miranda Icon Pack";
		public String description = "Miranda Icon Pack for %proto%";
		@SuppressWarnings("deprecation")
		public String copyright = "Copyright © Author Name, " + (new Date().getYear() + 1900);
		public String version = "1.0.0.0";
	}
	
	public static class Config
	{
		public final Paths paths = new Paths();
		public final DllInfo dllInfo = new DllInfo();
	}
	
	public static void main(String[] args)
	{
		initVelocity();
		
		cfg = new Config();
		Display display = new Display();
		final Shell shell = new Shell(display);
		shell.setLayout(new GridLayout(1, true));
		
		final JfgFormComposite form = new JfgFormComposite(shell, SWT.NONE, new JfgFormData(JfgFormData.DIALOG));
		form.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		form.addContentsFrom(new ReflectionGroup(cfg));
		
		createMenu(shell, form);
		
		Button ok = new Button(shell, SWT.PUSH);
		ok.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		ok.addListener(SWT.Selection, new Listener() {
			public void handleEvent(Event event)
			{
				form.copyToModel();
				
				try
				{
					process();
				}
				catch (Exception e)
				{
					e.printStackTrace();
					MessageBox mb = new MessageBox(shell, SWT.OK | SWT.ERROR);
					mb.setText("Error");
					mb.setMessage(e.getMessage());
					mb.open();
				}
			}
		});
		ok.setText("Generate");
		
		out = new Text(shell, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
		out.setLayoutData(new GridData(GridData.FILL_BOTH));
		
		shell.setText("Miranda Ico Generator Config");
		shell.setDefaultButton(ok);
		shell.pack();
		shell.setSize(500, 500);
		shell.open();
		while (!shell.isDisposed())
		{
			if (!display.readAndDispatch())
				display.sleep();
		}
		display.dispose();
	}
	
	private static void createMenu(final Shell shell, final JfgFormComposite form)
	{
		Menu bar = new Menu(shell, SWT.BAR);
		shell.setMenuBar(bar);
		{
			MenuItem toolsItem = new MenuItem(bar, SWT.CASCADE);
			toolsItem.setText("&Tools");
			Menu submenu = new Menu(shell, SWT.DROP_DOWN);
			toolsItem.setMenu(submenu);
			
			MenuItem item = new MenuItem(submenu, SWT.PUSH);
			item.addListener(SWT.Selection, new Listener() {
				public void handleEvent(Event e)
				{
					form.copyToModel();
					
					new RcToIcons().show(shell, cfg.paths.pluginsPath);
				}
			});
			item.setText("Convert .rc to .icons ...");
			
			item = new MenuItem(submenu, SWT.PUSH);
			item.addListener(SWT.Selection, new Listener() {
				public void handleEvent(Event e)
				{
					form.copyToModel();
					
					new ImportSkinIcons().show(shell, cfg.paths.pluginsPath);
				}
			});
			item.setText("Import SkinIcons (exported from DBEditor) ...");
		}
		
		{
			MenuItem helpItem = new MenuItem(bar, SWT.CASCADE);
			helpItem.setText("&Help");
			Menu submenu = new Menu(shell, SWT.DROP_DOWN);
			helpItem.setMenu(submenu);
			
			MenuItem item = new MenuItem(submenu, SWT.PUSH);
			item.addListener(SWT.Selection, new Listener() {
				public void handleEvent(Event e)
				{
					
				}
			});
			item.setText("&About...");
		}
	}
	
	protected static void process() throws IOException, InterruptedException
	{
		out.append("Starting process...\n\n");
		
		List<String> protos = readProtos();
		List<Overlay> overlays = readOverlays();
		List<Plugin> plugins = readPlugins(true);
		
		for (String proto : protos)
		{
			createStatusDir();
			
			out.append(proto + "\n");
			
			for (Overlay overlay : overlays)
			{
				out.append("   - " + overlay.getName() + " : ");
				
				File outIco = getOutIco(proto, overlay);
				
				File orig = getProtoIco(proto + "-" + overlay.getName());
				if (orig.exists())
				{
					out.append("found in prots folder\n");
					copy(outIco, orig);
					continue;
				}
				
				File protoIco = getProtoIco(proto);
				if (!protoIco.exists())
				{
					out.append("could not find " + getFullPath(protoIco) + "\n");
					continue;
				}
				
				if (overlay.isEmpty())
				{
					out.append("could not find overlay, using original icon\n");
					copy(outIco, protoIco);
					continue;
				}
				
				out.append("merging icons\n");
				createIco(outIco, protoIco, overlay);
			}
			
			out.append("   - Building dll\n");
			buildDll(proto, overlays);
			
			out.append("\n");
		}
		
		out.append("Building plugins...\n\n");
		
		for (Plugin plugin : plugins)
		{
			createPluginsDir();
			
			out.append(plugin.getName() + "\n");
			out.append("   - Building dll\n");
			buildDll(plugin);
			
			out.append("\n");
		}
		
		out.append("Building preview\n");
		
		buildPreview(getPreviewFile(), protos, overlays, plugins);
		
		out.append("\n");
		out.append("Done");
	}
	
	private static void createStatusDir()
	{
		File dir = new File(cfg.paths.outPath, "status");
		if (!dir.exists() && !dir.mkdirs())
			throw new IllegalStateException("Could not create the output path");
	}
	
	private static void createPluginsDir()
	{
		File dir = getPluginsDir();
		if (!dir.exists() && !dir.mkdirs())
			throw new IllegalStateException("Could not create the output path");
	}
	
	private static File getPreviewFile()
	{
		return new File(cfg.paths.outPath, "preview.png");
	}
	
	private static void buildPreview(File preview, List<String> protos, List<Overlay> overlays, List<Plugin> plugins) throws IOException
	{
		List<String> titles = new ArrayList<String>();
		titles.add(cfg.dllInfo.name + " " + cfg.dllInfo.version);
		titles.add(cfg.dllInfo.copyright);
		
		int ICO_SIZE = 16;
		int SPACE = 4;
		int BORDER = 10;
		int FONT_SIZE = 10;
		boolean FONT_BOLD = false;
		int TITLE_SPACE = 20;
		int TITLE_FONT_SIZE = 10;
		boolean TITLE_FONT_BOLD = true;
		
		Point maxTextSize = getMaxTextSize(protos, plugins, FONT_SIZE, FONT_BOLD);
		int TEXT_MAX_WIDTH = maxTextSize.x + BORDER;
		int TEXT_HEIGHT = maxTextSize.y;
		int LINE_HEIGHT = max(TEXT_HEIGHT, ICO_SIZE);
		
		maxTextSize = getMaxTextSize(titles, null, TITLE_FONT_SIZE, TITLE_FONT_BOLD);
		int TITLE_HEIGHT = maxTextSize.y;
		
		Display display = Display.getCurrent();
		
		int pluginsSpace = 0;
		for (Plugin plugin : plugins)
		{
			int rows = plugin.getExistingIcons() / overlays.size();
			if (plugin.getExistingIcons() % overlays.size() != 0)
				rows++;
			pluginsSpace += rows * (LINE_HEIGHT + SPACE);
		}
		
		Image img = new Image(display, BORDER + TEXT_MAX_WIDTH + overlays.size() * (ICO_SIZE + SPACE) + BORDER, BORDER + TITLE_HEIGHT * 2
				+ SPACE + TITLE_SPACE + protos.size() * (LINE_HEIGHT + SPACE) + pluginsSpace - SPACE + BORDER);
		
		GC gc = new GC(img);
		gc.setBackground(display.getSystemColor(SWT.COLOR_WHITE));
		
		int top = BORDER;
		
		Font titleFont = setFont(gc, TITLE_FONT_SIZE, TITLE_FONT_BOLD);
		
		for (String text : titles)
		{
			int textHeight = gc.stringExtent(text).y;
			gc.drawText(text, BORDER, top + (LINE_HEIGHT - textHeight) / 2, true);
			top += TITLE_HEIGHT + SPACE;
		}
		top += TITLE_SPACE - SPACE;
		
		Font font = setFont(gc, FONT_SIZE, FONT_BOLD);
		titleFont.dispose();
		
		for (String proto : protos)
		{
			int left = BORDER;
			
			int textHeight = gc.stringExtent(proto).y;
			gc.drawText(proto, left, top + (LINE_HEIGHT - textHeight) / 2, true);
			
			left += TEXT_MAX_WIDTH + SPACE;
			
			for (Overlay overlay : overlays)
			{
				Image ico = ImageUtils.loadImage(getOutIco(proto, overlay), ICO_SIZE, ICO_SIZE, false);
				if (ico != null)
				{
					ImageData data = ico.getImageData();
					
					gc.drawImage(ico, 0, 0, data.width, data.height, left, top + (LINE_HEIGHT - min(ICO_SIZE, data.height)) / 2, min(
							ICO_SIZE, data.width), min(ICO_SIZE, data.height));
					
					ico.dispose();
				}
				else
				{
					out.append("[ERROR] Error loading ico " + getFullPath(getOutIco(proto, overlay)) + "\n");
				}
				
				left += ICO_SIZE + SPACE;
			}
			
			top += LINE_HEIGHT + SPACE;
		}
		
		for (Plugin plugin : plugins)
		{
			List<PluginIcon> icons = plugin.getIcons();
			int rows = plugin.getExistingIcons() / overlays.size();
			if (plugin.getExistingIcons() % overlays.size() != 0)
				rows++;
			
			int left = BORDER;
			
			int textHeight = gc.stringExtent(plugin.getName()).y;
			gc.drawText(plugin.getName(), left, top + (LINE_HEIGHT - textHeight) / 2, true);
			
			for (int i = 0, icoNum = 0; i < rows && icoNum < icons.size(); i++)
			{
				left = BORDER + TEXT_MAX_WIDTH + SPACE;
				
				for (int j = 0; j < overlays.size() && icoNum < icons.size();)
				{
					PluginIcon icon = icons.get(icoNum++);
					if (!icon.exists())
						continue;
					
					Image ico = ImageUtils.loadImage(icon.getFile(), false);
					if (ico != null)
					{
						ImageData data = ico.getImageData();
						
						gc.drawImage(ico, 0, 0, data.width, data.height, left, top + (LINE_HEIGHT - min(ICO_SIZE, data.height)) / 2, min(
								ICO_SIZE, data.width), min(ICO_SIZE, data.height));
						
						ico.dispose();
					}
					else
					{
						out.append("[ERROR] Error loading ico " + getFullPath(icon.getFile()) + "\n");
					}
					j++;
					left += ICO_SIZE + SPACE;
				}
				
				top += LINE_HEIGHT + SPACE;
			}
		}
		
		font.dispose();
		gc.dispose();
		
		ImageLoader loader = new ImageLoader();
		loader.data = new ImageData[] { img.getImageData() };
		loader.save(getFullPath(preview), SWT.IMAGE_PNG);
		
		img.dispose();
	}
	
	private static Point getMaxTextSize(List<String> protos, List<Plugin> plugins, int fontHeight, boolean bold)
	{
		Display display = Display.getCurrent();
		Image img = new Image(display, 1, 1);
		
		GC gc = new GC(img);
		
		Font font = setFont(gc, fontHeight, bold);
		
		Point ret = new Point(0, 0);
		for (String proto : protos)
		{
			Point extent = gc.stringExtent(proto);
			ret.x = max(ret.x, extent.x);
			ret.y = max(ret.y, extent.y);
		}
		if (plugins != null)
		{
			for (Plugin plugin : plugins)
			{
				Point extent = gc.stringExtent(plugin.getName());
				ret.x = max(ret.x, extent.x);
				ret.y = max(ret.y, extent.y);
			}
		}
		
		font.dispose();
		gc.dispose();
		img.dispose();
		
		return ret;
	}
	
	private static Font setFont(GC gc, int height, boolean bold)
	{
		Font initialFont = gc.getFont();
		FontData[] fontData = initialFont.getFontData();
		for (int i = 0; i < fontData.length; i++)
		{
			fontData[i].setHeight(height);
			if (bold)
				fontData[i].setStyle(fontData[i].getStyle() | SWT.BOLD);
			else
				fontData[i].setStyle(fontData[i].getStyle() & ~SWT.BOLD);
		}
		Font font = new Font(Display.getCurrent(), fontData);
		gc.setFont(font);
		return font;
	}
	
	private static File getOutIco(String proto, Overlay overlay)
	{
		return findIcon(new File(new File(cfg.paths.outPath, "status"), proto + "-" + overlay.getName()));
	}
	
	private static void createIco(File outIco, File protoIco, Overlay overlay) throws IOException
	{
		Image img = loadImage(protoIco, 16, 16);
		BufferedImage proto = convertToAWT(img.getImageData());
		img.dispose();
		
		overlay.blend(proto);
		
		outIco.getParentFile().mkdirs();
		if (outIco.exists())
			outIco.delete();
		ICOEncoder.write(proto, outIco);
	}
	
	public static void copy(File dest, File orig) throws IOException
	{
		if (!orig.exists())
			throw new FileNotFoundException(getFullPath(orig));
		if (dest.exists())
			dest.delete();
		
		Copy copy = new Copy();
		copy.setFile(orig);
		copy.setTofile(dest);
		copy.execute();
	}
	
	private static File getProtoIco(String name)
	{
		return findIcon(new File(cfg.paths.protosPath, name));
	}
	
	private static List<Overlay> readOverlays()
	{
		List<Overlay> ret = new ArrayList<Overlay>();
		ret.add(new Overlay(cfg, "offline"));
		ret.add(new Overlay(cfg, "online"));
		ret.add(new Overlay(cfg, "away"));
		ret.add(new Overlay(cfg, "dnd"));
		ret.add(new Overlay(cfg, "na"));
		ret.add(new Overlay(cfg, "occupied"));
		ret.add(new Overlay(cfg, "freechat"));
		ret.add(new Overlay(cfg, "invisible"));
		ret.add(new Overlay(cfg, "onthephone"));
		ret.add(new Overlay(cfg, "outtolunch"));
		return ret;
	}
	
	private static List<String> readProtos()
	{
		File[] files = cfg.paths.protosPath.listFiles(new FilenameFilter() {
			public boolean accept(File dir, String name)
			{
				int length = name.length();
				if (length < 5)
					return false;
				if (name.indexOf('-') >= 0)
					return false;
				
				String ext = name.substring(length - 4);
				return indexOf(imageExts, ext) >= 0;
			}
		});
		Set<String> ret = new TreeSet<String>();
		for (int i = 0; i < files.length; i++)
		{
			File f = files[i];
			String name = f.getName();
			ret.add(name.substring(0, name.length() - 4));
		}
		return new ArrayList<String>(ret);
	}
	
	public static List<Plugin> readPlugins(boolean onlyIfHasAnIcon)
	{
		File[] files = cfg.paths.pluginsPath.listFiles(new FileFilter() {
			public boolean accept(File pathname)
			{
				return pathname.getName().endsWith(".icons");
			}
		});
		List<Plugin> ret = new ArrayList<Plugin>();
		for (int i = 0; i < files.length; i++)
		{
			File f = files[i];
			
			String name = f.getName().substring(0, f.getName().length() - 6);
			
			Plugin p = new Plugin(name, f.getParentFile());
			if (onlyIfHasAnIcon && p.getExistingIcons() < 1)
				continue;
			
			ret.add(p);
		}
		return ret;
	}
	
	private static void initVelocity()
	{
		try
		{
			Properties props = new Properties();
			props.put("runtime.log.logsystem.class", VelocityLogger.class.getName());
			props.put("file.resource.loader.class", VelocityResourceLoader.class.getName());
			props.put("velocimacro.permissions.allow.inline.local.scope", "true");
			Velocity.init(props);
		}
		catch (Exception e)
		{
			e.printStackTrace();
			throw new RuntimeException(e);
		}
	}
	
	private static void buildDll(Plugin plugin)
	{
		File dir = getPluginsDir();
		
		File dllFile = new File(dir, plugin.getName() + ".dll");
		if (dllFile.exists())
			dllFile.delete();
		
		File rcFile = new File(dir, plugin.getName() + ".rc");
		
		mergeTemplate(plugin, rcFile);
		
		String rc = getFullPath(rcFile);
		rc = rc.substring(0, rc.length() - 3);
		
		File utils = new File("utils");
		File buildme = new File(utils, "buildme.bat");
		if (!buildme.exists())
			throw new IllegalStateException("Could not find utils dir");
		
		ExecTask exec = new ExecTask();
		exec.setExecutable(getFullPath(buildme));
		exec.createArg().setValue(rc);
		exec.setDir(utils);
		exec.setFailonerror(true);
		exec.setLogError(true);
		exec.execute();
	}
	
	private static File getPluginsDir()
	{
		return new File(cfg.paths.outPath, "plugins");
	}
	
	private static void mergeTemplate(Plugin plugin, File file)
	{
		File templateName = new File("utils/plugin.rc.vm");
		try
		{
			String pluginName = plugin.getName();
			
			VelocityContext context = new VelocityContext();
			context.put("plugin", encode(pluginName));
			context.put("description", encode(cfg.dllInfo.description.replace("%proto%", pluginName)));
			context.put("name", encode(cfg.dllInfo.name.replace("%proto%", pluginName)));
			context.put("copyright", encode(cfg.dllInfo.copyright.replace("%proto%", pluginName)));
			
			String[] version = cfg.dllInfo.version.split("\\.");
			for (int j = 0; j < 4; j++)
				context.put("v" + (j + 1), encode(Integer.toString(version.length >= j ? toInt(version[j]) : 0)));
			
			List<VelocityIcon> icons = new ArrayList<VelocityIcon>();
			for (PluginIcon icon : plugin.getIcons())
			{
				if (!icon.exists())
					out.append("[WARN] " + pluginName + ": Icon not found: " + icon.getName() + "\n");
				else
					icons.add(new VelocityIcon(Integer.toString(icon.getId()), encode(getFullPath(icon.getFile()))));
			}
			context.put("icons", icons);
			
			Template template = Velocity.getTemplate(getFullPath(templateName));
			
			File parentFile = file.getParentFile();
			if (!parentFile.exists() && !parentFile.mkdirs())
				throw new IllegalStateException("Could not create the output path");
			
			BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(file), "ISO-8859-1"));
			try
			{
				template.merge(context, writer);
			}
			finally
			{
				writer.close();
			}
		}
		catch (Exception e)
		{
			throw new RuntimeException(e);
		}
	}
	
	public static class VelocityIcon
	{
		private final String id;
		private final String file;
		
		public VelocityIcon(String id, String file)
		{
			this.id = id;
			this.file = file;
		}
		
		public String getId()
		{
			return id;
		}
		
		public String getFile()
		{
			return file;
		}
	}
	
	private static void buildDll(String proto, List<Overlay> overlays) throws IOException, InterruptedException
	{
		File dir = new File(cfg.paths.outPath, "status");
		
		File dllFile = new File(dir, "proto_" + proto + ".dll");
		if (dllFile.exists())
			dllFile.delete();
		
		File rcFile = new File(dir, "proto_" + proto + ".rc");
		
		mergeTemplate(proto, overlays, rcFile);
		
		String rc = getFullPath(rcFile);
		rc = rc.substring(0, rc.length() - 3);
		
		File utils = new File("utils");
		File buildme = new File(utils, "buildme.bat");
		if (!buildme.exists())
			throw new IllegalStateException("Could not find utils dir");
		
		ExecTask exec = new ExecTask();
		exec.setExecutable(getFullPath(buildme));
		exec.createArg().setValue(rc);
		exec.setDir(utils);
		exec.setFailonerror(true);
		exec.setLogError(true);
		exec.execute();
	}
	
	private static void mergeTemplate(String proto, List<Overlay> overlays, File file)
	{
		File templateName = new File("utils/autobuild.rc.vm");
		try
		{
			VelocityContext context = new VelocityContext();
			context.put("proto", encode(proto));
			context.put("description", encode(cfg.dllInfo.description.replace("%proto%", proto)));
			context.put("name", encode(cfg.dllInfo.name.replace("%proto%", proto)));
			context.put("copyright", encode(cfg.dllInfo.copyright.replace("%proto%", proto)));
			
			String[] version = cfg.dllInfo.version.split("\\.");
			for (int j = 0; j < 4; j++)
				context.put("v" + (j + 1), encode(Integer.toString(version.length >= j ? toInt(version[j]) : 0)));
			
			for (Overlay overlay : overlays)
				context.put(overlay.getName(), encode(getFullPath(getOutIco(proto, overlay))));
			
			Template template = Velocity.getTemplate(getFullPath(templateName));
			
			File parentFile = file.getParentFile();
			if (!parentFile.exists() && !parentFile.mkdirs())
				throw new IllegalStateException("Could not create the output path");
			
			BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(file), "ISO-8859-1"));
			try
			{
				template.merge(context, writer);
			}
			finally
			{
				writer.close();
			}
		}
		catch (Exception e)
		{
			throw new RuntimeException(e);
		}
	}
	
	private static int toInt(String str)
	{
		try
		{
			return Integer.parseInt(str);
		}
		catch (NumberFormatException e)
		{
			return 0;
		}
	}
	
	private static String encode(String str)
	{
		return str.replace("\\", "\\\\").replace("\"", "\\\\\"");
	}
}
